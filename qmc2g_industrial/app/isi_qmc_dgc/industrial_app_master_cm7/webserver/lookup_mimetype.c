/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "constants.h"
#include <strings.h>
#include <search.h>

/**
 * @brief simple key value type
 */
typedef struct plug_kv{
    const char*key;
    const char*value;
}plug_kv_t;


/**
 * @brief case insensitive comparison function
 *
 * @param pa string a
 * @param pb string b
 *
 * @return  0 on equal or -1 / 1 depending on ordering
 */
static int case_insensitive(const void * pa, const void * pb) {
    const plug_kv_t *a=pa, *b=pb;
    return strcasecmp(a->key, b->key);
}

/**
 * @brief ** SORTED ** table of extensions to mimetype string mappings
 */
static const plug_kv_t s_mimetypes[]= {
		{ "7z", "application/x-7z-compressed" },
		{ "asc", "application/pgp-keys" },
		{ "avi", "video/x-msvideo" },
		{ "bin", "application/octet-stream" },
		{ "buffer", "application/octet-stream" },
		{ "bz2", "application/x-bzip2" },
		{ "class", "application/java-vm" },
		{ "conf", "text/plain" },
		{ "css", "text/css" },
		{ "csv", "text/csv" },
		{ "der", "application/x-x509-ca-cert" },
		{ "doc", "application/msword" },
		{ "docm", "application/vnd.ms-word.document.macroenabled.12" },
		{ "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
		{ "dotm", "application/vnd.ms-word.template.macroenabled.12" },
		{ "dotx", "application/vnd.openxmlformats-officedocument.wordprocessingml.template" },
		{ "dtd", "application/xml-dtd" },
		{ "dump", "application/octet-stream" },
		{ "exe", "application/octet-stream" },
		{ "gz", "application/gzip" },
		{ "htm", "text/html" },
		{ "html", "text/html" },
		{ "ico", "image/vnd.microsoft.icon" },
		{ "img", "application/octet-stream" },
		{ "ini", "text/plain" },
		{ "install", "application/x-install-instructions" },
		{ "jar", "application/java-archive" },
		{ "jpeg", "image/jpeg" },
		{ "jpg", "image/jpeg" },
		{ "js", "application/javascript" },
		{ "json", "application/json" },
		{ "json5", "application/json5" },
		{ "list", "text/plain" },
		{ "log", "text/plain" },
		{ "m4a", "audio/mp4" },
		{ "m4v", "video/x-m4v" },
		{ "map", "application/json" },
		{ "md", "text/markdown" },
		{ "mp3", "audio/mp3" },
		{ "otf", "font/otf" },
		{ "p10", "application/pkcs10" },
		{ "p12", "application/x-pkcs12" },
		{ "p7b", "application/x-pkcs7-certificates" },
		{ "p7s", "application/pkcs7-signature" },
		{ "pdf", "application/pdf" },
		{ "pem", "application/x-x509-ca-cert" },
		{ "png", "image/png" },
		{ "svg", "image/svg+xml" },
		{ "svgz", "image/svg+xml" },
		{ "text", "text/plain" },
		{ "ttf", "font/ttf" },
		{ "txt", "text/plain" },
		{ "wasm", "application/wasm" },
		{ "xhtml", "application/xhtml+xml" },
		{ "yaml", "text/yaml" },
		{ "zip", "application/zip" },
};


/**
 * @brief lookup the mimetype for an extension
 *
 * use binary search to locate the extension in the key value table
 *
 * @param ext extension string
 *
 * @return mimetype string or NULL
 */
const char* lookup_mimetype(const char* ext){
	plug_kv_t key={.key=ext,.value=NULL};
	plug_kv_t *result;
	result= bsearch(&key, s_mimetypes , sizeof(s_mimetypes)/sizeof(s_mimetypes[0]), sizeof(s_mimetypes[0]), case_insensitive);
	return result?result->value:"application/octet-stream";
}
