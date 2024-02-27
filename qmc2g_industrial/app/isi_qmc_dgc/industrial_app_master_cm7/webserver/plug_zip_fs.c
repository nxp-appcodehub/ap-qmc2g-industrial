/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "arch/cc.h"
#include "cit_httpd_opt.h"
#include "plug.h"
#include <stdint.h>
#include <string.h>
#include <search.h>
#include <lwip/mem.h>

#include "contrib/miniz.h"

#include "plug_zip_fs.h"

static char PLUG_TMP_BUFFER_SECTION s_plug_zip_buffer[TCP_SND_BUF];

#if PLUG_ZIP_FS_FATFS
#include "ff.h"
FILE *plug_zip_open_fatfs(FIL *fp, const char *path);
#endif

enum zip_fs_match
{
    NO_MATCH,
    COMPRESSED,
    UNCOMPRESSED,
};

struct zip_fs_state
{
    struct file_info *info;
    mz_zip_archive *zip;
    mz_zip_reader_extract_iter_state *iterator;
    enum zip_fs_match match;
};

PLUG_EXTENSION(plug_zip_fs, struct zip_fs_state);

/**
 * @brief array of rules that use plug_zip_fs
 */
static const plug_rule_t *s_rules[PLUG_ZIP_FILE_COUNT];

/**
 * @brief array of zip file structs for rules to use
 */
static mz_zip_archive s_archives[PLUG_ZIP_FILE_COUNT];

#if PLUG_ZIP_FS_FATFS
/**
 * @brief array of fatfs file handles for zip files
 */
static FIL s_fatfs_file[PLUG_ZIP_FILE_COUNT];
#endif

/**
 * @brief array search tree root nodes for the zip file contents
 */
static void *s_fs_roots[PLUG_ZIP_FILE_COUNT] = {0};

/**
 * @brief file metadata extracted from the zip file, added to the root search tree
 */
struct file_info
{
    char *path;
    const char *mimetype;
    u32_t file_index;
    u32_t comp_size;
    u32_t uncomp_size;
    char etag[12];
};

/**
 * @brief  path comparison function
 *
 * @param a path one
 * @param b path two
 *
 * @return  strcmp result
 */
static int path_compare(const void *a, const void *b)
{
    struct file_info *fa, *fb;
    fa = (struct file_info *)a;
    fb = (struct file_info *)b;
    return strcmp(fa->path, fb->path);
}

/**
 * @brief loop up the correct zip file on rule match
 */
http_status_t plug_zip_fs_on_match(plug_state_p state, const plug_rule_t *rule, plug_request_t *request)
{
    int i;
    for (i = 0; i < PLUG_ZIP_FILE_COUNT; i++)
    {
        if (s_rules[i] == rule)
        {
            break;
        }
    }
    if (i < PLUG_ZIP_FILE_COUNT && (strlen(request->path) < MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE))
    {
        state->match         = NO_MATCH;
        void **fs_root       = s_fs_roots + i;
        void *ptr            = NULL;
        struct file_info key = {0};
        key.path             = request->path + 1;
        ptr                  = tfind(&key, fs_root, path_compare);
        if (ptr)
        {
            state->info = *(struct file_info **)ptr;
            state->zip  = s_archives + i;
            return HTTP_REPLY_OK;
        }
    }
    /* file not found, return flags as status code or -1 to skip the rule */
    if (rule->flags)
    {
        return -abs(rule->flags);
    }
    else
    {
        return -1; /* skip rule */
    }
}

/**
 * @brief just populate the file's meta data in case of a check_resource call
 */
http_status_t plug_zip_fs_on_check_resource(plug_state_p state,
                                            const plug_rule_t *rule,
                                            plug_request_t *request,
                                            plug_response_t *response,
                                            http_status_t status_code)
{
    if (state->info)
    {
        response->content_type = state->info->mimetype;
        response->etag         = state->info->etag;
        return HTTP_REPLY_OK;
    }
    return status_code;
}

http_status_t plug_zip_fs_on_reply(plug_state_p state,
                                   const plug_rule_t *rule,
                                   plug_request_t *request,
                                   plug_response_t *response,
                                   http_status_t status_code)
{
    if (state->info)
    {
        mz_uint flags          = 0;
        response->content_type = state->info->mimetype;
        response->etag         = state->info->etag;
        if (state->info->comp_size && request->accept_deflate)
        {
            state->match             = COMPRESSED;
            response->content_length = state->info->comp_size;
            flags |= MZ_ZIP_FLAG_COMPRESSED_DATA;
        }
        else
        {
            state->match             = UNCOMPRESSED;
            response->content_length = state->info->uncomp_size;
        }
        state->iterator = mz_zip_reader_extract_iter_new(state->zip, state->info->file_index, flags);
        return HTTP_REPLY_OK;
    }
    if (rule->flags)
    {
        return -abs(rule->flags);
    }
    else
    {
        return HTTP_ACCEPT; /* no match */
    }
}

#define DEFLATE_HEADER "Content-Encoding: deflate\r\n"
#define STATIC_HEADERS "Cache-Control: public, max-age=259200, must-revalidate\r\n"

/**
 * @brief write content encoding and cache control headers based on file metadata
 *
 */
void plug_zip_fs_write_headers(plug_state_p state,
                               const plug_rule_t *rule,
                               plug_request_t *request,
                               plug_response_t *response,
                               http_status_t status_code,
                               FILE *headers)
{
    switch (state->match)
    {
        case COMPRESSED:
            fputs(DEFLATE_HEADER, headers);
            /*fallthrough*/
        case UNCOMPRESSED:
            fputs(STATIC_HEADERS, headers);
            /*fallthrough*/
        default:
            break;
    }
}

/**
 * @brief send the file contents to the client, either decompressed or compressed
 *
 */
void plug_zip_fs_write_body_data(plug_state_p state,
                                 const plug_rule_t *rule,
                                 plug_request_t *request,
                                 const plug_response_t *response,
                                 http_status_t status_code,
                                 FILE *body,
                                 u16_t sndbuf)
{
    u16_t len;
    if (state->iterator)
    {
        sndbuf = MIN(sndbuf, sizeof(s_plug_zip_buffer));
        len    = mz_zip_reader_extract_iter_read(state->iterator, s_plug_zip_buffer, sndbuf);
        if (len > 0)
        {
            fwrite(s_plug_zip_buffer, len, 1, body);
        }
    }
}

/**
 * @brief  close zip reader state
 */
void plug_zip_fs_on_cleanup(plug_state_p state,
                            const plug_rule_t *rule,
                            const plug_request_t *request,
                            const plug_response_t *response,
                            http_status_t status_code)
{
    if (state->iterator)
    {
        mz_zip_reader_extract_iter_free(state->iterator);
        state->iterator = NULL;
    }
}

/**
 * @brief malloc for use with miniz
 * ensure __malloc_lock is implemented on multi tasking systems!
 */
static void *zip_alloc(void *opaque, size_t items, size_t size)
{
    void *ptr = NULL;
    (void)opaque;
    unsigned long long bytes = (unsigned long long)items * size;
    if (bytes <= SIZE_MAX)
    {
        ptr = malloc((size_t)bytes);
    }
    return ptr;
}

/**
 * @brief free for use with miniz
 * ensure __malloc_lock is implemented on multi tasking systems!
 */
static void zip_free(void *opaque, void *address)
{
    (void)opaque, (void)address;
    free(address);
}

/**
 * @brief realloc for use with miniz
 * ensure __malloc_lock is implemented on multi tasking systems!
 */
static void *zip_realloc(void *opaque, void *address, size_t items, size_t size)
{
    void *ptr = NULL;
    (void)opaque;
    unsigned long long bytes = (unsigned long long)items * size;
    if (bytes <= SIZE_MAX)
    {
        ptr = realloc(address, (size_t)bytes);
    }
    return ptr;
}

/**
 * @brief initialize the plug
 * creates a metadata search tree for all zip files
 *
 * allocates the metadata structures once.
 *
 */
void plug_zip_fs_initializer(const plug_rule_t *rule)
{
    struct plug_fs_info (*get_info)(void) = rule->options;
    struct plug_fs_info info              = get_info();
    static int zip_index                  = 0;
    if (zip_index < PLUG_ZIP_FILE_COUNT)
    {
        mz_zip_archive *zip = &s_archives[zip_index];
        s_rules[zip_index]  = rule;
        mz_zip_zero_struct(zip);

        zip->m_pAlloc   = zip_alloc;
        zip->m_pRealloc = zip_realloc;
        zip->m_pFree    = zip_free;

#if PLUG_ZIP_FS_FATFS
        FIL *fp = &s_fatfs_file[zip_index];
        if (info.size == 0)
        {
            FILE *f;
            f = plug_zip_open_fatfs(fp, (const char *)info.data);
            if (!f || !mz_zip_reader_init_cfile(zip, f, f_size(fp), 0))
            {
                LWIP_PLATFORM_DIAG(("zip fatfs init failed"));
                return;
            }
        }
        else
        {
#endif
            if (!mz_zip_reader_init_mem(zip, info.data, info.size, MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY))
            {
                LWIP_PLATFORM_DIAG(("zip init failed"));
                return;
            }
#if PLUG_ZIP_FS_FATFS
        }
#endif
        mz_uint i, file_count;
        const mz_uint index_count = mz_zip_reader_get_num_files(zip);

        mz_zip_archive_file_stat stat;

        for (i = 0, file_count = 0; i <= index_count; i++)
        {
            mz_zip_clear_last_error(zip);
            mz_zip_reader_file_stat(zip, i, &stat);
            if (mz_zip_get_last_error(zip) || stat.m_is_directory || !stat.m_is_supported)
            {
                continue;
            }
            file_count++;
        }

        struct file_info *list = mem_calloc(file_count, sizeof(struct file_info));

        for (i = 0, file_count = 0; i <= index_count; i++)
        {
            mz_zip_clear_last_error(zip);
            mz_zip_reader_file_stat(zip, i, &stat);
            if (mz_zip_get_last_error(zip) || stat.m_is_directory || !stat.m_is_supported)
            {
                continue;
            }

            struct file_info *fi = list + file_count++;
            char *extension;

            extension = strrchr(stat.m_filename, '.') + 1;

            fi->path        = strdup(stat.m_filename);
            fi->comp_size   = (stat.m_method == MZ_DEFLATED) ? stat.m_comp_size : 0;
            fi->uncomp_size = stat.m_uncomp_size;
            fi->file_index  = stat.m_file_index;
            fi->mimetype    = lookup_mimetype(extension);

            u16_t elen = snprintf(fi->etag, sizeof(fi->etag), "\"%08X\"", stat.m_crc32);
            LWIP_ASSERT("etag buffer overflow", elen < sizeof(fi->etag));

            /* insert to search tree */
            tsearch(fi, s_fs_roots + zip_index, path_compare);
        }

        zip_index++;
    }
    return;
}
