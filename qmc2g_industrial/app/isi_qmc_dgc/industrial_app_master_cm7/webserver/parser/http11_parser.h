/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef http11_parser_h
#define http11_parser_h
#include <stdbool.h>
#include <stdint.h>
#include <cmsis_compiler.h>

typedef void (*element_cb)(void *data, uint16_t at, uint16_t len);
typedef void (*field_cb)(
    void *data, uint32_t fhash, uint16_t fpos, uint16_t flen, uint16_t vpos, uint16_t vlen);

typedef struct http11_parser
{
    void *data;
    bool uri_relaxed;
    element_cb request_method;
    element_cb request_uri;
    element_cb fragment;
    element_cb request_path;
    element_cb query_string;
    element_cb http_version;
    element_cb header_done;
    field_cb http_field;
    uint16_t global_offset;
    uint16_t content_length;
    uint16_t content_len;
    uint16_t cs;
    uint16_t body_start;
    uint16_t mark;
    uint16_t field_start;
    uint16_t field_len;
    uint32_t field_hash;
    uint16_t query_start;
    bool xml_sent;
    bool json_sent;
} http11_parser;

int http11_parser_init(http11_parser *parser);
int http11_parser_finish(http11_parser *parser);
uint16_t http11_parser_execute(http11_parser *parser, const char *buffer, uint16_t len, uint16_t off);
int http11_parser_has_error(http11_parser *parser);
int http11_parser_is_finished(http11_parser *parser);

#define http_parser_nread(parser) (parser)->nread

#endif
