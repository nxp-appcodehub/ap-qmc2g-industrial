/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "cit_httpd.h"
#include "cit_httpd_opt.h"
#include "constants.h"
#include "err.h"
#include "fsl_common.h"
#include "plug.h"
#include <lwip/arch.h>
#include <lwip/tcpbase.h>
#include <lwip/api.h>
#include <lwip/memp.h>
#include <lwip/pbuf.h>
#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/stats.h>
#include <lwip/altcp.h>
#ifdef LWIP_ALTCP_TLS
#include <lwip/altcp_tls.h>
#endif
#include <lwip/tcp.h>
#include <lwip/altcp_tcp.h>
#include <lwip/mem.h>
#include <mbedtls/certs.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include "parser/http11_parser.h"
#include "contrib/pattern.h"

#include "urldecode.h"

#ifndef LWIP_TCP
#error LWIP_TCP is required
#endif

#ifndef LWIP_CALLBACK_API
#error LWIP_CALLBACK_API is reqired
#endif

#ifndef LWIP_ALTCP
#error LWIP_ALTCP is required
#endif

char PLUG_TMP_BUFFER_SECTION httpd_temp_buffer[TCP_SND_BUF];
/**
 * @brief number of poll interval tics for some seconds
 */
#define POLL_TIMEOUT(seconds) ((u16_t)((seconds * 2) / CIT_HTTPD_POLL_INTERVAL) + 1)
#define KEEP_ALIVE_COUNT      POLL_TIMEOUT(CIT_HTTPD_KEEP_ALIVE_TIMEOUT)
#define TIMEOUT_COUNT         POLL_TIMEOUT(CIT_HTTPD_SESSION_TIMEOUT)

#define STRINGIFY_SYMBOL(x) #x
#define STRINGIFY(x)        STRINGIFY_SYMBOL(x)

#define TIMEOUTSTRING STRINGIFY(CIT_HTTPD_KEEP_ALIVE_TIMEOUT)
enum
{
    HASH_100_CONTINUE = 0xbb706528u
};

/*
 * fallback rule in case of a parse error
 * */
static const plug_rule_t generate_status_rule = PLUG_RULE("", HTTP_ANY, plug_status, 0, NULL);

/* states the connection can be in */
enum cit_httpd_session_state
{
    CS_NONE = 0, /* Not initialized/illegal */
    CS_ACCEPTED,
    CS_HEADER,
    CS_BODY,
    CS_REPLY, /* Reply Header */
    CS_CLOSING,
};

#define NO_MATCH -1

static err_t cit_httpd_poll(void *arg, struct altcp_pcb *conn);

/* server connection state */
struct cit_httpd_state
{
    plug_rule_match_t active_match[CIT_HTTPD_MAX_ACTIVE_RULES];
    plug_rule_match_t error_match;
    plug_rule_match_t *body_match;

    enum cit_httpd_session_state state;
    const struct cit_httpd_config *config;

    struct pbuf *header_pbuf; /* http header pbuf */
    struct pbuf *body_pbuf;   /* http body pbuf */

    FILE *tcp_fd;
    err_t write_err;

    http11_parser parser;

    plug_request_t request;
    plug_response_t response;

    u32_t response_body_remaining_bytes;
    u32_t request_body_remaining_bytes;

    u16_t remaining_request_size;
    u16_t session_timeout;

    u16_t status_code;
    u16_t keep_alive : 1;
};

LWIP_MEMPOOL_DECLARE(cit_httpd_pool, CIT_HTTPD_MAX_SESSIONS, sizeof(struct cit_httpd_state), "cit_httpd");

/**
 * @brief u32_t saturating subtraction
 *
 * @param m minuend
 * @param s subtrahend
 *
 * @returns m-s or 0 on overflow
 */

inline u32_t sat_sub_u32(u32_t m, u32_t s)
{
    u32_t sat_diff = m - s;
    sat_diff &= -(sat_diff <= m);
    return sat_diff;
}

/**
 * @brief u16_t saturating subtraction
 *
 * @param m minuend
 * @param s subtrahend
 *
 * @returns m-s or 0 on overflow
 */

inline u16_t sat_sub_u16(u16_t m, u16_t s)
{
    u16_t sat_diff = m - s;
    sat_diff &= -(sat_diff <= m);
    return sat_diff;
}

/**
 * @brief u16_t saturating addition
 *
 * @param a summand
 * @param b summand
 *
 * @returns m+s or 0xffff on overflow
 */

inline u16_t sat_sum_u16(u16_t a, u16_t b)
{
    u16_t sat_sum = a + b;
    sat_sum |= -(sat_sum < a);
    return sat_sum;
}

/**
 * @brief release a server instance
 *
 * @param httpd state pointer
 */
static void cit_httpd_free(struct cit_httpd_state *httpd)
{
    fclose(httpd->tcp_fd);
    if (httpd->header_pbuf)
    {
        pbuf_free(httpd->header_pbuf);
    }
    if (httpd->body_pbuf)
    {
        pbuf_free(httpd->body_pbuf);
    }
    LWIP_MEMPOOL_FREE(cit_httpd_pool, httpd);
    /* unlock the underlying pcb if this is the last server state */
}

/**
 * @brief cleanup a server state to reuse in a new request
 *
 * @param httpd  server state
 *
 * @return lwip error code
 */
static void cit_httpd_cleanup(struct cit_httpd_state *httpd)
{
    plug_rule_match_t *m;
    s16_t index;
    for (index = 0; index < CIT_HTTPD_MAX_ACTIVE_RULES; index++)
    {
        m = &httpd->active_match[index];
        if (!m->rule)
        {
            break;
        }
        if (m->rule->plug->on_cleanup_fn)
        {
            m->rule->plug->on_cleanup_fn(&m->state, m->rule, &httpd->request, &httpd->response, httpd->status_code);
        }
        if (m->rule->plug->fields_vec)
        {
            const plug_header_t *const *fields;
            /* iterate it */
            for (fields = m->rule->plug->fields_vec; *fields; ++fields)
            {
                const plug_header_t *const f = *fields;
                if (*f->is_set == false)
                {
                    continue;
                }
                *f->is_set = false;
            }
        }
        if( m->rule == httpd->error_match.rule){
        	httpd->error_match.rule=NULL;
        };
    }
    if (httpd->error_match.rule && httpd->error_match.rule->plug && httpd->error_match.rule->plug->on_cleanup_fn)
    {
        httpd->error_match.rule->plug->on_cleanup_fn(&httpd->error_match.state, httpd->error_match.rule, &httpd->request, &httpd->response, httpd->status_code);
    }
    httpd->session_timeout = KEEP_ALIVE_COUNT;
}

/**
 * @brief close a server session
 *
 * @param conn   pcb
 * @param httpd  server state
 *
 * @return lwip error code
 *
 */
static err_t cit_httpd_close(struct altcp_pcb *conn, struct cit_httpd_state *httpd, bool abort)
{
    httpd->state = CS_NONE;
    altcp_recv(conn, NULL);
    altcp_poll(conn, NULL, CIT_HTTPD_POLL_INTERVAL);
    altcp_sent(conn, NULL);
    altcp_err(conn, NULL);
    altcp_arg(conn, NULL);

    if (httpd->header_pbuf)
    {
        altcp_recved(conn, httpd->header_pbuf->tot_len);
        pbuf_free(httpd->header_pbuf);
        httpd->header_pbuf = NULL;
    }
    if (httpd->body_pbuf)
    {
        altcp_recved(conn, httpd->body_pbuf->tot_len);
        pbuf_free(httpd->body_pbuf);
        httpd->body_pbuf = NULL;
    }
    if (!abort)
    {
        altcp_close(conn);
    }
    else
    {
        altcp_abort(conn);
    }
    cit_httpd_cleanup(httpd);
    cit_httpd_free(httpd);
    return abort ? ERR_ABRT : ERR_OK;
}

typedef struct pbuf_fd
{
    struct pbuf *p;
    size_t pos;
    size_t size;
} pbuf_fd_t;

/**
 * @brief drop bytes from the front of a pbuf chain
 *
 * @param p pbuf
 * @param drop_len bytes to drop
 *
 * @return  new pbuf pointer
 */
struct pbuf *drop_processed_head(struct pbuf *p, u16_t drop_len)
{
    u16_t offset;
    struct pbuf *pn;

    pn = pbuf_skip(p, drop_len, &offset);
    LWIP_ASSERT("pbuf offset out of range", drop_len <= p->tot_len);
    drop_len = MIN(drop_len, p->tot_len);
    if (pn == p)
    {
        pbuf_remove_header(pn, offset);
    }
    else
    {
        if (pn)
        {
            pbuf_ref(pn);
            pbuf_remove_header(pn, offset);
        }
        pbuf_free(p);
    }
    return pn;
}

/**
 * @brief file io callback for fseek
 *
 * @param fd  file descriptor
 * @param offset offset
 * @param whence SEEK mode
 *
 * @return fpos_t value
 */
static fpos_t body_seek_handler(void *fd, off_t offset, int whence)
{
    pbuf_fd_t *pf = fd;
    if (!pf->p)
    {
        return -1;
    }
    size_t size = pf->size;
    size_t pos  = pf->pos;
    switch (whence)
    {
        case SEEK_SET:
            pf->pos = MIN(size, MAX(0, offset));
            break;
        case SEEK_CUR:
            pos     = MAX(0, pf->pos + offset);
            pf->pos = MIN(pos, size);
            break;
        case SEEK_END:
            pos     = MAX(0, size + offset);
            pf->pos = MIN(pos, size);
            break;
        default:
            break;
    }
    return pf->pos;
}

/**
 * @brief file io handler for fread
 *
 * @param fd file descriptor
 * @param buf target buffer
 * @param nbytes bytes to write
 *
 * @return read return value
 */
static int body_read_handler(void *fd, char *buf, int nbytes)
{
    pbuf_fd_t *pf = fd;
    struct pbuf *p;
    u16_t offset;
    int rc = -1;
    if (pf->p && nbytes > 0)
    {
        rc = 0;
        if (pf->pos < pf->size)
        {
            p = pbuf_skip(pf->p, pf->pos, &offset);
            if (p)
            {
                rc = MIN(pf->size - pf->pos, nbytes);
                rc = pbuf_copy_partial(p, buf, rc, offset);
                pf->pos += rc;
            }
        }
    }
    return rc;
}

/**
 * @brief file io handler for close
 *
 * @param fd file descriptor
 *
 * @return  returns 0
 */
static int body_close_handler(void *fd)
{
    pbuf_fd_t *pf = fd;
    pf->p         = NULL;
    return 0;
}

/**
 * @brief file io handler for write
 *
 * writing to the tcp connection
 *
 * @param fd file descriptor
 * @param buf     buffer
 * @param nbytes  number of bytes in the buffer
 *
 * @return  number of bytes written or -1 in case of an error
 */
static int plug_tcp_write_handler(void *fd, const char *buf, int nbytes)
{
    struct altcp_pcb *conn = fd;
    int rc                 = -1;
    if (conn && conn->arg)
    {
        struct cit_httpd_state *httpd = conn->arg;
        u16_t bytecount               = altcp_sndbuf(conn);
        u8_t flags                    = TCP_WRITE_FLAG_COPY;
        err_t err                     = ERR_OK;

        // ensure nbytes is positive and limit to sndbuf size
        bytecount = MIN(MAX(0, nbytes), bytecount);

        httpd->response_body_remaining_bytes = sat_sub_u32(httpd->response_body_remaining_bytes, bytecount);

        if (httpd->response_body_remaining_bytes > 0)
        {
            flags |= TCP_WRITE_FLAG_MORE;
        }

        if (bytecount > 0)
        {
            err = altcp_write(conn, buf, bytecount, flags);
        }
        httpd->write_err = err;
        rc               = err != ERR_OK ? -1 : bytecount;
    }
    return rc;
}

#define KEEP_ALIVE_HEADERS       \
    "Connection: keep-alive\r\n" \
    "Keep-Alive: timeout=" TIMEOUTSTRING ", max=32\r\n"
#define CLOSE_CONNECTION_HEADERS "Connection: Close\r\n"

/**
 * @brief rule matching context
 */
struct rule_match_ctx
{
    const char *last_match;
    const char *root_match;
    const char *match_end;
};

/**
 * @brief match a rule against a request
 *
 * @param ctx      matching context
 * @param request  httpd request
 * @param rule     the rule to match against
 *
 * @return true if rule matches, false otherwise
 */
static bool plug_match_rule(struct rule_match_ctx *ctx, plug_request_t *request, const plug_rule_t *rule)
{
    const char *match_string;
    u16_t len;
    match_string = request->path;
#if ENABLE_RULE_MATCH_INTERFACE || ENABLE_RULE_MATCH_PORT
    char buf[6];
#endif
#if ENABLE_RULE_MATCH_INTERFACE
    struct netif *interface;
#endif

    int rule_methods    = rule->methods;
    const char *pattern = rule->pattern;

    bool negate         = false;
    bool is_local_match = false;
    bool is_and_match   = false;
    bool is_or_match    = false;

    int request_method = request->method;

    /* GET implies HEAD */
    if (rule_methods & HTTP_GET)
    {
        rule_methods |= HTTP_HEAD;
    }

    /* check if there is a pattern */
    if (!pattern || (rule_methods & request_method) == 0)
    {
        return false;
    }

    /* check pattern prefixes and load the match_string accordingly */
    switch (pattern[0])
    {
        /* logic prefix */
        case '&': /* AND match only if previous rule matched */
            is_and_match = 1;
            pattern++;
            break;
        case '|': /* OR match only if previous rule did not match */
            is_or_match = 1;
            pattern++;
            break;
        default:
            break;
    }
    /* negate match prefix */
    switch (pattern[0])
    {
        case '!': /* negate match result */
            negate = !negate;
            pattern++;
            break;
        default:
            break;
    }

    /* pattern source prefix */
    switch (pattern[0])
    {
        /* relative path , continue match at the end of the last pattern */
        case '.':
            /* if there is no match we are done */
            if (!ctx->root_match)
            {
                return false;
            }
            match_string   = ctx->root_match;
            is_local_match = true;
            pattern++;
            break;
#if ENABLE_RULE_MATCH_URI
        case '=': /* match full URI */
            match_string = request->uri;
            pattern++;
            break;
#endif
#if ENABLE_RULE_MATCH_PORT
        case ':': /* match port number */
            snprintf(buf, sizeof(buf), "%u", request->local_port);
            match_string = buf;
            pattern++;
            break;
#endif
#if ENABLE_RULE_MATCH_INTERFACE
        case '@': /* match network interface name, (en0, en1) */
            interface = ip_route(request->local_ip, request->remote_ip);
            snprintf(buf, sizeof(buf), "%2s%u", interface->name, interface->num);
            match_string = buf;
            pattern++;
            break;
#endif
#if ENABLE_RULE_MATCH_QUERYSTRING
        case '?': /* match query string (only after a successful match!) */
            match_string = request->query_string;
            pattern++;
            break;
#endif
#if ENABLE_RULE_MATCH_FRAGMENT
        case '#': /* match fragment (only after a successful match!) */
            match_string = request->fragment;
            pattern++;
            break;
#endif
        case '/':
        default: /* match anywhere in the path */
            match_string = request->path;
            break;
    }
    if (is_and_match && !ctx->last_match)
    {
        ctx->last_match = NULL;
    }
    else if (is_or_match && ctx->last_match)
    {
        ctx->last_match = NULL;
    }
    else
    {
        /* perform pattern matching */
        len = strlen(match_string);
        /* ctx->last_match = pattern_match(match_string, len, pattern); */
        ctx->match_end = pattern_match(match_string, len, pattern);
        /* if matching fails but the rule is negated return the start of the match string */
        if (!ctx->match_end && negate)
        {
            ctx->last_match = match_string;
            ctx->match_end  = match_string;
        }
        else
        {
            ctx->last_match = ctx->match_end;
        }
    }
    /* root match (not local) */
    if (!is_local_match)
    {
        ctx->root_match = ctx->match_end;
        /* local matching, pattern matched and completed the input (end of string) */
    }
    else if (ctx->match_end && ctx->match_end[0] == '\0')
    {
        ctx->root_match = NULL; /* end root_matching */
    }

    return !!ctx->match_end;
}

/**
 * @brief execute a ruleset on the httpd server state
 *
 * @param httpd     server state
 * @param rule_vec  rule array pointer
 * @param rule_cnt  number of rules
 */
static void plug_process_rules(struct cit_httpd_state *httpd, const plug_rule_t *rule_vec, u16_t rule_cnt)
{
    const plug_rule_t *const re = rule_vec + rule_cnt;
    const plug_rule_t *r;
    plug_rule_match_t *m;
    struct rule_match_ctx ctx = {0};

    s16_t match_slot = 0;

    memset(&httpd->active_match, 0, sizeof(httpd->active_match));
    for (r = rule_vec; r < re && match_slot < CIT_HTTPD_MAX_ACTIVE_RULES; r++)
    {
        m = &httpd->active_match[match_slot];
        /* if a rule matches and it is a plug (not only a pattern rule) */
        if (plug_match_rule(&ctx, &httpd->request, r) && r->plug)
        {
            const plug_extension_t *plug = r->plug;
            m->state                     = (abstract_plug_state_t){0};
            m->rule                      = r;
            m->match_end                 = ctx.match_end;
            /* call any match handler, get an state pointer */
            if (plug->on_match_fn)
            {
                httpd->request.path_match_end = m->match_end;
                m->status_code                = plug->on_match_fn(&m->state, r, &httpd->request);
            }
            if (!plug->on_reply_fn && !plug->initializer_fn && !plug->on_cleanup_fn && !plug->write_headers_fn &&
                !plug->write_body_data_fn)
            {
                continue; /* reuse state, this rule will not respond; */
            }
            /* record last processed rule */
            if (plug->write_body_data_fn)
            {
                if (!plug->on_reply_fn)
                {
                    m->status_code = HTTP_REPLY_OK;
                }
            }
            else if (!plug->on_reply_fn)
            {
                m->status_code = NOREPLY_STATUS_CODE(m->status_code);
            }
            /* terminate, either with error or intention to respond */
            if (m->status_code <= HTTP_NOREPLY_CONTINUE || m->status_code >= HTTP_REPLY_CONTINUE)
            {
                httpd->body_match = m;
                break;
            }
            match_slot++; /* next match state  (and rule)*/
        }
    }
}

err_t cit_httpd_send_reply_continue(struct altcp_pcb *conn, struct cit_httpd_state *httpd);

/**
 * @brief send the reply to the request in the server state
 *
 * @param conn   pcb
 * @param httpd  server state
 *
 * @return lwip error code
 */
err_t cit_httpd_send_reply(struct altcp_pcb *conn, struct cit_httpd_state *httpd)
{
    /* call respond_fn */
    /* iterate rules */
    s16_t index;

    /* default (internal) error rule */
    httpd->error_match = (plug_rule_match_t){.rule = &generate_status_rule, .status_code = HTTP_ACCEPT, .state = {{0}}};
    httpd->write_err   = ERR_OK;

    plug_rule_match_t *m        = NULL;
    const plug_extension_t *ext = NULL;
    for (index = 0; index < CIT_HTTPD_MAX_ACTIVE_RULES; index++)
    {
        if (!httpd->active_match[index].rule)
        {
            break;
        }
        m                             = &httpd->active_match[index];
        httpd->request.path_match_end = m->match_end;
        ext                           = m->rule->plug;
        /* register any error handler */
        if ((m->rule->methods & HTTP_ERROR_HANDLER))
        {
            httpd->error_match = *m;
        }

        if (!ext->on_reply_fn && !ext->on_check_resource_fn)
        {
            if (httpd->request.if_none_match || httpd->request.if_match)
            {
              continue;
            }
        }

        if (ext->on_reply_fn && ! httpd->request.expect)
        {
            bool check_resource =
                (ext->on_check_resource_fn && (httpd->request.if_none_match || httpd->request.if_match));
            if (check_resource)
            {
                m->status_code =
                    ext->on_check_resource_fn(&m->state, m->rule, &httpd->request, &httpd->response, m->status_code);
            }
            else
            {
                m->status_code =
                    ext->on_reply_fn(&m->state, m->rule, &httpd->request, &httpd->response, m->status_code);
            }

            if (httpd->response.etag)
            {
                if (httpd->request.if_match && m->status_code >= HTTP_REPLY_OK &&
                    m->status_code < HTTP_REPLY_MULTIPLE_CHOICES)
                {
                    if (strncmp(httpd->request.etag, httpd->response.etag, sizeof(httpd->request.etag)) != 0)
                    {
                        httpd->response.content_length = 0;
                        m->status_code = HTTP_NOREPLY_EXPECTATION_FAILED;
                    }
                }
                else if (httpd->request.if_none_match && m->status_code >= HTTP_REPLY_OK &&
                    m->status_code < HTTP_REPLY_MULTIPLE_CHOICES)
                {
                    if (strncmp(httpd->request.etag, httpd->response.etag, sizeof(httpd->request.etag)) == 0)
                    {
                        httpd->response.content_length = 0;
                        if(httpd->request.method  == HTTP_GET || httpd->request.method == HTTP_HEAD)
                        {
                          m->status_code = HTTP_NOREPLY_NOT_MODIFIED;
                        }
                        else 
                        {
                          m->status_code = HTTP_NOREPLY_EXPECTATION_FAILED;
                        }
                    }
                }
            }
            /* if a resource check was used call on_reply if it was successful */
            if (check_resource && !(m->status_code < HTTP_NOREPLY_CONTINUE))
            {
                m->status_code =
                    ext->on_reply_fn(&m->state, m->rule, &httpd->request, &httpd->response, m->status_code);
            }
        }
        else if (httpd->request.expect && !(m->status_code <= HTTP_NOREPLY_CONTINUE))
        {
            if (httpd->request.expect_continue && ext->on_check_resource_fn)
            {
                m->status_code =
                    ext->on_check_resource_fn(&m->state, m->rule, &httpd->request, &httpd->response, m->status_code);
                if ((REPLY_STATUS_CODE(m->status_code) >= HTTP_REPLY_OK &&
                     REPLY_STATUS_CODE(m->status_code) < HTTP_REPLY_BAD_REQUEST))
                {
                    m->status_code = HTTP_NOREPLY_CONTINUE;
                }
            }
            else
            {
                m->status_code = HTTP_NOREPLY_EXPECTATION_FAILED;
            }
        }
        if (REPLY_STATUS_CODE(m->status_code) >= HTTP_REPLY_CONTINUE)
        {
            break;
        }
    }

    if (!m->status_code)
    {
    	m->status_code = HTTP_NOREPLY_INTERNAL_SERVER_ERROR;
    }
    if (!m->rule || m->status_code < HTTP_ACCEPT || !ext->write_body_data_fn)
    {
        plug_rule_match_t *h            = &httpd->error_match;
        const plug_rule_t *rule         = httpd->error_match.rule;
        const plug_extension_t *handler = rule->plug;
        m->status_code                  = NOREPLY_STATUS_CODE(m->status_code);
        if (handler->on_match_fn)
            handler->on_match_fn(&h->state, rule, &httpd->request);
        if (handler->on_reply_fn)
            handler->on_reply_fn(&h->state, rule, &httpd->request, &httpd->response, m->status_code);
        httpd->body_match = &httpd->error_match;
    }
    else
    {
        httpd->body_match = m;
    }

    /* ensure TCP_WRITE_FLAG_MORE gets set by plug_tcp_write_handler, the length of the full header is not known */
    httpd->response_body_remaining_bytes = UINT_MAX;

    /* generate response */
    httpd->status_code=REPLY_STATUS_CODE(m->status_code);
    const char *status_string = status_code_string(httpd->status_code);
    fprintf(httpd->tcp_fd, "HTTP/1.1 %d %s\r\n", httpd->status_code, status_string ? status_string : "");

    for (index = 0; index < CIT_HTTPD_MAX_ACTIVE_RULES; index++)
    {
        plug_rule_match_t *rm = NULL;
        if (!httpd->active_match[index].rule)
        {
            break;
        }
        rm                             = &httpd->active_match[index];
        ext                           = rm->rule->plug;
        httpd->request.path_match_end = rm->match_end;
        if (ext->write_headers_fn)
        {
            rm->rule->plug->write_headers_fn(&rm->state, rm->rule, &httpd->request, &httpd->response, httpd->status_code,
                                            httpd->tcp_fd);
        }
        if (REPLY_STATUS_CODE(rm->status_code) >= HTTP_REPLY_CONTINUE)
        {
            break;
        }
    }

    /* generate default headers for fields in the response struct */
    if (httpd->response.content_type)
    {
        fprintf(httpd->tcp_fd, "Content-Type: %s\r\n", httpd->response.content_type);
    }
    fprintf(httpd->tcp_fd, "Content-Length: %u\r\n", (unsigned)httpd->response.content_length);

    if (httpd->response.etag)
    {
        fprintf(httpd->tcp_fd, "ETag: %s\r\n", httpd->response.etag);
    }

    /* generate keep-alive headers */
    if (httpd->keep_alive == 1)
    {
        fwrite(KEEP_ALIVE_HEADERS, sizeof(KEEP_ALIVE_HEADERS) - 1, 1, httpd->tcp_fd);
    }
    else
    {
        fwrite(CLOSE_CONNECTION_HEADERS, sizeof(CLOSE_CONNECTION_HEADERS) - 1, 1, httpd->tcp_fd);
    }
    fputs("\r\n", httpd->tcp_fd);

    fflush(httpd->tcp_fd);

    /* check if there should be a body produced... */
    /* (there is a match,and a plug, it's not a HEAD request, and no Expect: 100-continue */
    if (httpd->request.method == HTTP_HEAD || httpd->request.expect)
    {
        httpd->response_body_remaining_bytes = 0;
    }
    else
    {
        httpd->response_body_remaining_bytes = httpd->response.content_length;
    }

    return cit_httpd_send_reply_continue(conn, httpd);
}

/**
 * @brief send the reply to the request in the server state
 *
 * @param conn   pcb
 * @param httpd  server state
 *
 * @return lwip error code
 */
err_t cit_httpd_send_reply_continue(struct altcp_pcb *conn, struct cit_httpd_state *httpd)
{
    plug_rule_match_t *m = httpd->body_match;
    /* if no content remains call plug to generate more if still more is needed */
    if (m && httpd->response_body_remaining_bytes > 0)
    {
        const plug_rule_t *rule     = m->rule;
        const plug_extension_t *ext = rule->plug;
        /* generate body data until all is generated, we can't send more, or there are no buffers */
        {
            ext->write_body_data_fn(&m->state, rule, &httpd->request, &httpd->response, httpd->status_code, httpd->tcp_fd,
                                    altcp_sndbuf(conn));
        }
        fflush(httpd->tcp_fd);
    }
    /* if no content remains prepare next request */
    if (httpd->response_body_remaining_bytes == 0)
    {
        httpd->state = httpd->keep_alive && httpd->request.keep_alive ? CS_ACCEPTED : CS_CLOSING;
        if (httpd->state == CS_ACCEPTED)
        {
            cit_httpd_cleanup(httpd);
        }
        else
        {
            return cit_httpd_close(conn, httpd, false);
        }
    }
    return ERR_OK;
}

/**
 * @brief receive and process body data
 *
 * @param conn   pcb
 * @param httpd  server state
 *
 * @return lwip error code
 */
static err_t cit_httpd_process_body(struct altcp_pcb *conn, struct cit_httpd_state *httpd)
{
    err_t err = ERR_OK;
    LWIP_ASSERT("process body in wrong state", httpd->state == CS_BODY);

    if (httpd->body_pbuf && httpd->body_pbuf->tot_len > 0)
    {
        struct pbuf *p  = httpd->body_pbuf;
        u32_t chain_len = 0;
        do
        {
            chain_len += p->len;
            p = p->next;
        } while (p && chain_len < httpd->request_body_remaining_bytes);
        chain_len = LWIP_MIN(chain_len, httpd->request_body_remaining_bytes);

        bool did_use_body_data = false;
        plug_rule_match_t *m;
        s16_t index;
        for (index = 0; index < CIT_HTTPD_MAX_ACTIVE_RULES; index++)
        {
            m = &httpd->active_match[index];
            if (!m->rule || httpd->request_body_remaining_bytes == 0)
                break;
            /* pass body data to the plug handling the body */
            bool can_process_body = m && m->rule->plug->on_body_data_fn && !(m->status_code <= HTTP_NOREPLY_CONTINUE);
            /* if there is a match, a on_body_data handler and no negative status code */
            if (can_process_body)
            {
                pbuf_fd_t fd    = {.pos = 0, .p = httpd->body_pbuf, .size = chain_len};
                FILE *body_file = funopen(&fd, body_read_handler, NULL, body_seek_handler, body_close_handler);
                if (body_file)
                {
                    /* call on_body_data_fn */
                    int rc = m->rule->plug->on_body_data_fn(&m->state, m->rule, &httpd->request, &httpd->response,
                                                            body_file, httpd->body_pbuf->tot_len);
                    fclose(body_file);
                    /* any consumed body bytes are removed */
                    /* if a plug seeks back, it will be called again later */
                    m->status_code = rc;
                }
                else
                {
                    m->status_code = HTTP_NOREPLY_INTERNAL_SERVER_ERROR;
                }
                if (m->status_code >= HTTP_ACCEPT && fd.pos > 0)
                {
                    did_use_body_data = true;
                    /* mark as processed */
                    altcp_recved(conn, fd.pos);
                    httpd->request_body_remaining_bytes = sat_sub_u32(httpd->request_body_remaining_bytes, fd.pos);
                    httpd->body_pbuf                    = drop_processed_head(httpd->body_pbuf, fd.pos);
                    // if the body is complete but the pbuf is not empty it's part of the next request
                    if (httpd->request_body_remaining_bytes == 0)
                    {
                        httpd->header_pbuf = httpd->body_pbuf;
                        httpd->body_pbuf   = NULL;
                        break;
                    }
                    if (m->status_code != HTTP_ACCEPT)
                    {
                        break;
                    }
                }
            }
        }
        if (!did_use_body_data)
        {
            /* mark as processed */
            altcp_recved(conn, httpd->body_pbuf->tot_len);
            /* drop the body data */
            httpd->request_body_remaining_bytes = sat_sub_u32(httpd->request_body_remaining_bytes, chain_len);
            httpd->body_pbuf                    = drop_processed_head(httpd->body_pbuf, chain_len);
            // if the body is complete but the pbuf is not empty it's part of the next request
            if (httpd->request_body_remaining_bytes == 0)
            {
                httpd->header_pbuf = httpd->body_pbuf;
                httpd->body_pbuf   = NULL;
            }
        }
    }
    /* go to reply once the body was consumed */
    if (httpd->request_body_remaining_bytes == 0)
    {
        httpd->state = CS_REPLY;
        err          = cit_httpd_send_reply(conn, httpd);
    }
    return err;
}

/**
 * @brief send an error response and close the request
 *
 * uses an internal ruleset to call plug_status to not rely an an working ruleset
 *
 * @param conn   pcb
 * @param httpd  server state
 * @param status_code HTTP status code
 *
 * @return lwip error code
 */
static err_t cit_httpd_error_response(struct altcp_pcb *conn, struct cit_httpd_state *httpd, http_status_t status_code)
{
    /* call error ruleset */
    plug_process_rules(httpd, &generate_status_rule, 1);
    httpd->body_match->status_code = status_code;
    /* free the request */
    if (httpd->header_pbuf)
    {
        pbuf_free(httpd->header_pbuf);
    }
    httpd->header_pbuf = NULL;

    /* process rest and reply now */
    httpd->state                         = CS_BODY;
    httpd->response_body_remaining_bytes = 0;
    httpd->status_code = REPLY_STATUS_CODE(status_code);
    err_t wr_err                         = cit_httpd_process_body(conn, httpd);
    altcp_output(conn);
    httpd->state = CS_CLOSING;
    return wr_err;
}

/**
 * @brief process and parse received header data
 *
 * @param conn   pcb
 * @param httpd  server state
 *
 * @return lwip error code
 */
static err_t cit_httpd_process_header(struct altcp_pcb *conn, struct cit_httpd_state *httpd)
{
    err_t wr_err   = ERR_OK;
    u16_t offset   = 0;
    struct pbuf *p = httpd->header_pbuf;

    /* get pbuf p with unparsed request data */
    if (httpd->parser.global_offset > 0)
    {
        p = pbuf_skip(httpd->header_pbuf, httpd->parser.global_offset, &offset);
    }

    /* the parser might need to be called multiple times, */
    /* it might keep some content unprocessed for one iteration */
    /* it will process each pbuf payload independently, there might be more than one. */
    while (httpd->state == CS_HEADER && p != NULL)
    {
        u16_t current_global_offset = httpd->parser.global_offset;
        http11_parser_execute(&httpd->parser, httpd->header_pbuf->payload, httpd->header_pbuf->len - offset, offset);
        if (http11_parser_has_error(&httpd->parser))
        {
            altcp_recved(conn, p->tot_len);
            LWIP_PLATFORM_DIAG(("HTTP parse error"));
            return cit_httpd_error_response(conn, httpd, HTTP_REPLY_BAD_REQUEST);
        }
        if (http11_parser_is_finished(&httpd->parser))
        {
            /* accept rest of the header and finish */
            u16_t diff = sat_sub_u16(httpd->parser.body_start,current_global_offset);
            altcp_recved(conn, diff);
            break;
        } else if (current_global_offset) {
            /* if the header is only partially received, accept the current offset difference */
            u16_t diff = sat_sub_u16(httpd->parser.global_offset, current_global_offset);
            altcp_recved(conn, diff);
        }
    }
    /* declare processed bytes as received */
    if (httpd->state == CS_BODY)
    {
        /* speed up response by directly processing the body */
        return cit_httpd_process_body(conn, httpd);
    }
    else
    {
        return wr_err;
    }
}

/**
 * @brief lwip error handler
 *
 * free the httpd server session
 *
 * @param arg   httpd server state
 */
static void cit_httpd_error(void *arg, err_t err)
{
    struct cit_httpd_state *httpd;

    LWIP_PLATFORM_DIAG(("HTTPD %s", lwip_strerr(err)));
    LWIP_UNUSED_ARG(err);

    httpd = (struct cit_httpd_state *)arg;
    if (httpd)
    {
        cit_httpd_cleanup(httpd);
        cit_httpd_free(httpd);
    }
}

/**
 * @brief lwip poll handler
 *
 * tries to continue any outstanding actions
 *
 * @param arg   httpd server state
 */

static err_t cit_httpd_poll(void *arg, struct altcp_pcb *conn)
{
    struct cit_httpd_state *httpd = (struct cit_httpd_state *)arg;
    if (!httpd)
    {
        return ERR_OK;
    }

    httpd->session_timeout = sat_sub_u16(httpd->session_timeout, 1);
    {
        switch (httpd->state)
        {
            case CS_ACCEPTED:
            case CS_BODY:
            case CS_HEADER:
                if (httpd->session_timeout == 0)
                {
                    return cit_httpd_close(conn, httpd, true);
                }

                break;
            case CS_CLOSING:
                return cit_httpd_close(conn, httpd, false);
                break;
            case CS_REPLY:
                if (httpd->session_timeout != 0)
                {
                    cit_httpd_send_reply_continue(conn, httpd);
                }
                else
                {
                    return cit_httpd_close(conn, httpd, false);
                }
                break;
            default:
                break;
        }
    }
    return ERR_OK;
}

/**
 * @brief lwip sent handler
 *
 * tries to continue any outstanding actions
 *
 * @param arg   httpd server state
 */
static err_t cit_httpd_sent(void *arg, struct altcp_pcb *conn, u16_t len)
{
    struct cit_httpd_state *httpd;
    httpd = (struct cit_httpd_state *)arg;

    altcp_sent(conn, cit_httpd_sent);

    /* this keeps the io pbufs in the io_pbuf_out chain around until they are sent. */

    switch (httpd->state)
    {
        case CS_REPLY:
            cit_httpd_send_reply_continue(conn, httpd);
            break;
        default:
            break;
    }
    return ERR_OK;
}

/**
 * @brief http parser header callback handler
 *
 * @param data   pointer to httpd state
 * @param fhash  field hash of the header
 * @param fpos   location of the field name
 * @param flen   length of the field name
 * @param vpos   location of the fields value
 * @param vlen   length of the fields value
 */
static void on_request_field(void *data, uint32_t fhash, uint16_t fpos, uint16_t flen, uint16_t vpos, uint16_t vlen)
{
    struct cit_httpd_state *httpd = data;
    char *s, *value = NULL;
    char buffer[128] = {0};
    /* these headers are processed for every request */
    switch (fhash)
    {
        case FIELD_HASH_CONTENT_LENGTH:
        {
            u16_t i;
            httpd->request.content_length = 0;
            for (i = vpos; i < vpos + vlen; i++)
            {
                int digit = pbuf_get_at(httpd->header_pbuf, i) - '0';
                if (digit < 10)
                {
                    httpd->request.content_length *= 10;
                    httpd->request.content_length += digit;
                }
                else
                {
                    httpd->request.content_length = 0;
                    break;
                }
            }
        }
        break;
        case FIELD_HASH_EXPECT:
            httpd->request.expect = true;
            /* validate 100-continue */
            if (vlen < sizeof(buffer))
            {
                pbuf_copy_partial(httpd->header_pbuf, buffer, vlen, vpos);
                buffer[vlen] = '\0';
                if (HASH_100_CONTINUE != field_hash((unsigned char *)buffer))
                {
                    httpd->request.expect_continue = true;
                }
            }

            break;
        case FIELD_HASH_CONTENT_TYPE:
            if (vlen < sizeof(buffer))
            {
                pbuf_copy_partial(httpd->header_pbuf, buffer, vlen, vpos);
                buffer[vlen]                     = '\0';
                httpd->request.content_type_hash = field_hash((unsigned char *)buffer);
            }
            break;
        case FIELD_HASH_CONNECTION:
            if (vlen < sizeof(buffer))
            {
                httpd->request.keep_alive = 0;
                pbuf_copy_partial(httpd->header_pbuf, buffer, vlen, vpos);
                if (strlen(buffer) == 10 && 0 == strncasecmp(buffer, "keep-alive", 10))
                {
                    httpd->request.keep_alive = 1; /* TODO: fix chrome issue */
                }
            }
            break;
        case FIELD_HASH_ACCEPT_ENCODING:
            httpd->request.accept_deflate = 0;
            if (vlen < sizeof(buffer))
            {
                buffer[vlen] = '\0';
                pbuf_copy_partial(httpd->header_pbuf, buffer, vlen, vpos);
                value = buffer;
                while ((s = strsep(&value, ", \r\n")))
                {
                    if (!s[0])
                    {
                        continue;
                    }
                    if (strlen(s) == 7 /* length of 'deflate' */ && 0 == strncasecmp(s, "deflate", 7))
                    {
                        httpd->request.accept_deflate = 1;
                    }
                }
            }
            break;
        case FIELD_HASH_IF_MATCH:
            if (vlen < sizeof(httpd->request.etag) && !httpd->request.if_match)
            {
                httpd->request.etag[vlen] = '\0';
                pbuf_copy_partial(httpd->header_pbuf, httpd->request.etag, vlen, vpos);
                httpd->request.if_match = 1;
            }
            break;
        case FIELD_HASH_IF_NONE_MATCH:
            if (vlen < sizeof(httpd->request.etag) && !httpd->request.if_none_match)
            {
                httpd->request.etag[vlen] = '\0';
                pbuf_copy_partial(httpd->header_pbuf, httpd->request.etag, vlen, vpos);
                httpd->request.if_none_match = 1;
            }
            break;
        case FIELD_HASH_ACCEPT:
            /* no longer assume html acceptance */
            httpd->request.accept_html = 0;
            if (vlen < sizeof(buffer))
            {
                buffer[vlen] = '\0';
                pbuf_copy_partial(httpd->header_pbuf, buffer, vlen, vpos);
                value = buffer;
                while ((s = strsep(&value, ",; \r\n")))
                {
                    if (s[0] == '\0')
                    {
                        continue; /* empty token */
                    }
                    if (s[0] == 'q' && s[1] == '=')
                    {
                        continue; /* ignore q= fields */
                    }
                    u32_t hash = field_hash((unsigned char *)s);
                    switch (hash)
                    {
                        case MIME_HASH_ALL:
                            httpd->request.accept_json = 1;
                            httpd->request.accept_html = 1;
                            break;
                        case MIME_HASH_TEXT_HTML:
                            httpd->request.accept_html = 1;
                            break;
                        case MIME_HASH_APPLICATION_JSON:
                            httpd->request.accept_json = 1;
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
    }

    s16_t index;
    for (index = 0; index < CIT_HTTPD_MAX_ACTIVE_RULES; index++)
    {
        plug_rule_match_t *m = &httpd->active_match[index];
        if (!m->rule)
            break;

        /* if there are registered headers */
        if (m->rule->plug->fields_vec)
        {
            /* get the registration list pointer */
            const plug_header_t *const *fields;

            /* iterate it */
            for (fields = m->rule->plug->fields_vec; *fields; ++fields)
            {
                const plug_header_t *const f = *fields;
                /* if this is the correct variable */
                if (f->fhash == fhash)
                {
                    uint16_t len;
                    /* ensure it's not already set (registrations are per plug, but iterate rules currently) */
                    if (*f->is_set)
                    {
                        continue;
                    }
                    *f->is_set = true;

                    /* copy any info requested into the buffers */
                    if (f->value_size)
                    {
                        len = MIN(vlen, f->value_size - 1);
                        pbuf_copy_partial(httpd->header_pbuf, f->value_ptr, len, vpos);
                        f->value_ptr[len] = '\0';
                    }
                    if (f->field_size)
                    {
                        len = MIN(flen, f->field_size - 1);
                        pbuf_copy_partial(httpd->header_pbuf, f->field_ptr, len, fpos);
                        f->field_ptr[len] = '\0';
                    }
                }
            }
        }
    }
}

/**
 * @brief http parser callback for the request method
 *
 * @param data  httpd server state
 * @param at    location of method string
 * @param len   length of the method string
 */
static void on_request_method(void *data, uint16_t at, uint16_t len)
{
    struct cit_httpd_state *httpd = data;
    char buffer[8];
    const char *str;
    str                   = pbuf_get_contiguous(httpd->header_pbuf, buffer, sizeof(buffer), len, at);
    httpd->request.method = lookup_http_method(str, len);
}

/**
 * @brief http parser callback for the request path
 *
 * @param data  httpd server state
 * @param at    location of path string
 * @param len   length of the path string
 */
static void on_request_path(void *data, uint16_t at, uint16_t len)
{
    struct cit_httpd_state *httpd = data;
    const u16_t size              = LWIP_MIN(len, sizeof(httpd->request.path) - 1);
    pbuf_copy_partial(httpd->header_pbuf, httpd->request.path, size, at);
    url_decode_inplace(httpd->request.path);
    httpd->request.path[size] = '\0';
}

/**
 * @brief http parser callback for the request query string
 *
 * @param data  httpd server state
 * @param at    location of query string
 * @param len   length of the query string
 */
static void on_request_query_string(void *data, uint16_t at, uint16_t len)
{
    struct cit_httpd_state *httpd = data;
    const u16_t size              = LWIP_MIN(len, sizeof(httpd->request.query_string) - 1);
    pbuf_copy_partial(httpd->header_pbuf, httpd->request.query_string, size, at);
    httpd->request.query_string[size] = '\0';
    url_decode_inplace(httpd->request.query_string);
}

/**
 * @brief http parser callback for the request fragment string
 *
 * @param data  httpd server state
 * @param at    location of fragment string
 * @param len   length of the fragment string
 */
static void on_request_fragment(void *data, uint16_t at, uint16_t len)
{
    struct cit_httpd_state *httpd = data;
    const u16_t size              = LWIP_MIN(len, sizeof(httpd->request.fragment) - 2);
    pbuf_copy_partial(httpd->header_pbuf, httpd->request.fragment, size, at);
    httpd->request.fragment[size + 1] = '\0';
    url_decode_inplace(httpd->request.fragment);
}

/**
 * @brief http parser callback for the request uri
 *
 * select the rules that will apply for this request, prior header processing
 *
 * @param data  httpd server state
 * @param at    location of query string
 * @param len   length of the query string
 */
static void on_request_uri(void *data, uint16_t at, uint16_t len)
{
    struct cit_httpd_state *httpd = data;
    const u16_t size              = LWIP_MIN(len, sizeof(httpd->request.uri) - 1);
    pbuf_copy_partial(httpd->header_pbuf, httpd->request.uri, size, at);
    httpd->request.uri[size] = '\0';
    /* process rules based on URI */
    plug_process_rules(httpd, httpd->config->rule_vec, httpd->config->rule_cnt);
}

/**
 * @brief http parser callback for completing header parsing
 *
 * drop header from pbuf, continue parsing any body data
 *
 * @param data  httpd server state
 * @param at    location of header string
 * @param len   length of the header string
 */
static void on_request_header_done(void *data, uint16_t at, uint16_t len)
{
    struct cit_httpd_state *httpd = data;

    /* header is done, switch to body mode */
    /* drop the header range from the pbuf, move the rest to the body pbuf pointer */
    httpd->body_pbuf                    = drop_processed_head(httpd->header_pbuf, httpd->parser.body_start);
    httpd->header_pbuf                  = NULL;
    httpd->request_body_remaining_bytes = httpd->request.content_length;

    httpd->state = CS_BODY;
}

/**
 * @brief lwip recv handler
 *
 * process new data arriving on the connection
 *
 * @param arg   httpd server state
 * @param conn  pcb
 * @param p     pbuf with new data
 * @param err   lwip error code
 */
static err_t cit_httpd_recv(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    struct cit_httpd_state *httpd;

    httpd = (struct cit_httpd_state *)arg;
    if (p == NULL)
    {
        return cit_httpd_close(conn, httpd, true);
    }

    // altcp_recved(conn, p->tot_len);
    switch (httpd->state)
    {
        case CS_ACCEPTED:
            /* just wait for a short while before aborting, keep-alive session termination */
            /* we could add a special case for the first connection */

            /* offer keep-alive */
            httpd->keep_alive = 1; /* TODO: fix chrome issue */
            /* but don't assume the client wants to */
            httpd->request.keep_alive = 0;
            /* assume html is ok, until the client states otherwise */
            httpd->request.accept_html = 1;
            /* data in p->payload, due to keep-alive/pipelining */
            /* there might be data already present, but not for new connections */
            httpd->request.expect          = 0;
            httpd->request.expect_continue = 0;
            memset(&httpd->request, 0, sizeof(httpd->request));
            memset(&httpd->response, 0, sizeof(httpd->response));
            http11_parser_init(&httpd->parser);
            httpd->state           = CS_HEADER;
            httpd->session_timeout = TIMEOUT_COUNT;
            /*fallthrough*/
        case CS_HEADER:
            /* fallthrough */
        case CS_REPLY:
            /* acknowledge data for header parsing, even if not fully processed */
            httpd->session_timeout        = TIMEOUT_COUNT;
            httpd->remaining_request_size = sat_sub_u16(httpd->remaining_request_size, p->tot_len);
            /* read some more data */
            if (httpd->header_pbuf)
            {
                pbuf_cat(httpd->header_pbuf, p);
            }
            else
            {
                httpd->header_pbuf = p;
            }
            p = NULL;
            break;
        case CS_BODY:
            httpd->session_timeout        = TIMEOUT_COUNT;
            httpd->remaining_request_size = sat_sub_u16(httpd->remaining_request_size, p->tot_len);
            /* read some more data */
            if (httpd->body_pbuf)
            {
                pbuf_cat(httpd->body_pbuf, p);
            }
            else
            {
                httpd->body_pbuf = p;
            }
            p = NULL;
            break;
        case CS_CLOSING:
        case CS_NONE:
        default:
            altcp_recved(conn, p->tot_len);
            pbuf_free(p);
            break;
    }
    switch (httpd->state)
    {
        case CS_HEADER:
        case CS_REPLY:
            return cit_httpd_process_header(conn, httpd);
        case CS_BODY:
            return cit_httpd_process_body(conn, httpd);
        default:
            return ERR_OK;
    }
}

/**
 * @brief lwip accept handler
 *
 * initialize a new connection and server state
 *
 * @param arg   httpd server state
 * @param conn connection pcb
 * @param err   lwip error code
 */
static err_t cit_httpd_accept(void *arg, struct altcp_pcb *conn, err_t err)
{
    err_t ret_err;
    struct cit_httpd_state *httpd = NULL;

    struct cit_httpd_config *conf = (struct cit_httpd_config *)arg;

    if (conn == NULL || err != ERR_OK)
    {
        return ERR_OK;
    }

    /* Unless this pcb should have NORMAL priority, set its priority now.
       When running out of pcbs, low priority pcbs can be aborted to create
       new pcbs of higher priority. */
    altcp_setprio(conn, CIT_HTTPD_TCP_PRIO);

    /* don't assume the socket is accepted yet, the tcp_pcb* is in conf->arg */
    tcp_backlog_delayed((struct tcp_pcb *)conf->arg);

    httpd = (struct cit_httpd_state *)LWIP_MEMPOOL_ALLOC(cit_httpd_pool);

    if (httpd == NULL)
    {
        /* no server available, everything is good, connection remains in the backlog */
        return ERR_OK;
    }
    else
    {
        memset(httpd, 0, sizeof(*httpd));

        httpd->state  = CS_ACCEPTED;
        httpd->config = conf;

        /* gather connection info*/
        httpd->request.remote_ip   = altcp_get_ip(conn, 0);
        httpd->request.local_ip    = altcp_get_ip(conn, 1);
        httpd->request.remote_port = altcp_get_port(conn, 0);
        httpd->request.local_port  = altcp_get_port(conn, 1);

        httpd->parser.data           = httpd;
        httpd->parser.http_field     = on_request_field;
        httpd->parser.request_uri    = on_request_uri;
        httpd->parser.request_path   = on_request_path;
        httpd->parser.request_method = on_request_method;
        httpd->parser.fragment       = on_request_fragment;
        httpd->parser.query_string   = on_request_query_string;
        httpd->parser.header_done    = on_request_header_done;
        httpd->session_timeout       = TIMEOUT_COUNT;

        /* open virtual file for writing to the connection */
        httpd->tcp_fd = funopen(conn, NULL, plug_tcp_write_handler, NULL, NULL);

        if (httpd->tcp_fd == NULL)
        {
            /* no descriptor available, free the server, don't accept */
            LWIP_MEMPOOL_FREE(cit_httpd_pool, httpd);
            return ERR_MEM;
        }

        setvbuf(httpd->tcp_fd, httpd_temp_buffer, _IOFBF, sizeof(httpd_temp_buffer));

        /* register the the callbacks for the new server state */
        altcp_arg(conn, httpd);
        altcp_recv(conn, cit_httpd_recv);
        altcp_err(conn, cit_httpd_error);
        altcp_poll(conn, cit_httpd_poll, CIT_HTTPD_POLL_INTERVAL);
        altcp_sent(conn, cit_httpd_sent);

        /* accept the connection, a server is available */
        tcp_backlog_accepted((struct tcp_pcb *)conf->arg);
        ret_err = ERR_OK;
    }
    return ret_err;
}

/**
 * @brief initialize the httpd server
 *
 * @param conf httpd configuration
 */
void cit_httpd_init(struct cit_httpd_config *conf)
{
    LWIP_MEMPOOL_INIT(cit_httpd_pool);
    const plug_rule_t *p, *pe;
    for (p = conf->rule_vec, pe = conf->rule_vec + conf->rule_cnt; p < pe; p++)
    {
        if (p->plug && p->plug->initializer_fn)
        {
            p->plug->initializer_fn(p);
        }
    }

    /* we need to create the basic tcp connection ourself, in order to be able to use the backlog */

    struct tcp_pcb *inner_pcb    = tcp_new_ip_type(conf->ip_type ? conf->ip_type : IPADDR_TYPE_ANY);
    struct altcp_pcb *httpd_tpcb = NULL;

    conf->arg = inner_pcb;
    if (!conf->ip_addr)
    {
        conf->ip_addr = IP_ADDR_ANY;
    }
    if (inner_pcb)
    {
        httpd_tpcb = altcp_tcp_wrap(inner_pcb);
    }
#if LWIP_ALTCP_TLS
    if (conf->tls && httpd_tpcb)
    {
        httpd_tpcb = altcp_tls_wrap(conf->tls, httpd_tpcb);
        if (!conf->port)
        {
            conf->port = 443;
        }
    }
#endif
    if (httpd_tpcb != NULL)
    {
        if (!conf->port)
        {
            conf->port = 80;
        }
        err_t err;
        altcp_arg(httpd_tpcb, conf);
        altcp_accept(httpd_tpcb, cit_httpd_accept);
        err = altcp_bind(httpd_tpcb, conf->ip_addr, conf->port);
        if (err == ERR_OK)
        {
            httpd_tpcb = altcp_listen_with_backlog(httpd_tpcb, CIT_HTTPD_MAX_SESSIONS);
        }
        else
        {
            LWIP_PLATFORM_DIAG(("%s: bind failed!", __func__, __LINE__));
            /* abort? output diagnostic? */
        }
    }
    else
    {
        LWIP_PLATFORM_DIAG(("%s: altcp failed!", __func__, __LINE__));
        /* abort? output diagnostic? */
    }
}
