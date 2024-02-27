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

#pragma GCC diagnostic ignored "-Wunused-const-variable"
#include "http11_parser.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#define LEN(AT, FPC) ((parser->global_offset+(uint16_t)(FPC-buffer)) - parser->AT)
#define MARK(M, FPC) (parser->M = (parser->global_offset+(uint16_t)(FPC-buffer)))
#define OFFSET(FPC)  (parser->global_offset + (uint16_t)((FPC)-buffer))
#define GET_MARK(F)  (parser->F)

/** Machine **/

%%{
  
  machine http11_parser;
  action mark {MARK(mark, fpc); }


  action start_field { MARK(field_start, fpc); parser->field_hash=5381; /*init djb2 hash*/ }
 
  # perform an djb2 hash on the lower case version of the characters
  action field_hash { 
      parser->field_hash=((parser->field_hash << 5) + parser->field_hash) + (uint32_t) tolower((int)fc); 
  }
  action write_field { 
    parser->field_len = LEN(field_start, fpc);
  }

  # just add digits together, multiply by 10 for each step
  #action update_content_length {
  #   parser->content_length *=10; 
  #   parser->content_length += (fc - '0');
  #}

  action start_value { MARK(mark, fpc); }

  action write_value {
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, 
        GET_MARK(field_hash) ,GET_MARK(field_start), parser->field_len, 
        GET_MARK(mark), LEN(mark, fpc));
    }
  }

  action request_method { 
    if(parser->request_method != NULL) 
      parser->request_method(parser->data, GET_MARK(mark), LEN(mark, fpc));
  }

  action request_uri { 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, GET_MARK(mark), LEN(mark, fpc));
  }

  action fragment {
    if(parser->fragment != NULL)
      parser->fragment(parser->data, GET_MARK(mark), LEN(mark, fpc));
  }

  action start_query {MARK(query_start, fpc); }
  action query_string { 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, GET_MARK(query_start), LEN(query_start, fpc));
  }

  action http_version {	
    if(parser->http_version != NULL)
      parser->http_version(parser->data, GET_MARK(mark), LEN(mark, fpc));
  }

  action request_path {
    if(parser->request_path != NULL)
      parser->request_path(parser->data, GET_MARK(mark), LEN(mark,fpc));
  }

  action done {
      /* if(parser->xml_sent || parser->json_sent) { */
      /*   parser->body_start = GET_MARK(mark); */
      /*   // +1 includes the \0 */
      /*   parser->content_len = OFFSET(fpc) - parser->body_start + 1; */
      /* } else { */
        parser->body_start = OFFSET(fpc)+ 1;

        if(parser->header_done != NULL) {
          parser->header_done(parser->data, OFFSET(fpc + 1), OFFSET(pe) - OFFSET(fpc) - 1);
        }
      /* } */
    fbreak;
  }

  #action xml {
  #    parser->xml_sent = 1;
  #}

  #action json {
  #    parser->json_sent = 1;
  #}


#### HTTP PROTOCOL GRAMMAR
  CRLF = ( "\r\n" | "\n" ) ;

  # URI description as per RFC 3986.

  more_delims   = ( "{" | "}" | "^" ) when { parser->uri_relaxed } ;
  sub_delims    = ( "!" | "$" | "&" | "'" | "(" | ")" | "*"
                  | "+" | "," | ";" | "=" | more_delims ) ;
  gen_delims    = ( ":" | "/" | "?" | "#" | "[" | "]" | "@" ) ;
  reserved      = ( gen_delims | sub_delims ) ;
  unreserved    = ( alpha | digit | "-" | "." | "_" | "~" ) ;

  pct_encoded   = ( "%" xdigit xdigit ) ;

  pchar         = ( unreserved | pct_encoded | sub_delims | ":" | "@" ) ;

  fragment      = ( ( pchar | "/" | "?" )* ) >mark %fragment ;

  query         = ( ( pchar | "/" | "?" )* ) %query_string ;

  # non_zero_length segment without any colon ":" ) ;
  segment_nz_nc = ( ( unreserved | pct_encoded | sub_delims | "@" )+ ) ;
  segment_nz    = ( pchar+ ) ;
  segment       = ( pchar* ) ;

  path_empty    = [] ;#( pchar{0} ) ;
  path_rootless = ( segment_nz ( "/" segment )* ) ;
  path_noscheme = ( segment_nz_nc ( "/" segment )* ) ;
  path_absolute = ( "/" ( segment_nz ( "/" segment )* )? ) ;
  path_abempty  = ( ( "/" segment )* ) ;

  path          = ( path_abempty    # begins with "/" or is empty
                  | path_absolute   # begins with "/" but not "//"
                  | path_noscheme   # begins with a non-colon segment
                  | path_rootless   # begins with a segment
                  | path_empty      # zero characters
                  ) ;

  reg_name      = ( unreserved | pct_encoded | sub_delims )* ;

  dec_octet     = ( digit                 # 0-9
                  | ("1"-"9") digit         # 10-99
                  | "1" digit{2}          # 100-199
                  | "2" ("0"-"4") digit # 200-249
                  | "25" ("0"-"5")      # 250-255
                  ) ;

  IPv4address   = ( dec_octet "." dec_octet "." dec_octet "." dec_octet ) ;
  h16           = ( xdigit{1,4} ) ;
  ls32          = ( ( h16 ":" h16 ) | IPv4address ) ;

  IPv6address   = (                               6( h16 ":" ) ls32
                  |                          "::" 5( h16 ":" ) ls32
                  | (                 h16 )? "::" 4( h16 ":" ) ls32
                  | ( ( h16 ":" ){1,} h16 )? "::" 3( h16 ":" ) ls32
                  | ( ( h16 ":" ){2,} h16 )? "::" 2( h16 ":" ) ls32
                  | ( ( h16 ":" ){3,} h16 )? "::"    h16 ":"   ls32
                  | ( ( h16 ":" ){4,} h16 )? "::"              ls32
                  | ( ( h16 ":" ){5,} h16 )? "::"              h16
                  | ( ( h16 ":" ){6,} h16 )? "::"
                  ) ;

  IPvFuture     = ( "v" xdigit+ "." ( unreserved | sub_delims | ":" )+ ) ;

  IP_literal    = ( "[" ( IPv6address | IPvFuture  ) "]" ) ;

  port          = ( digit* ) ;
  host          = ( IP_literal | IPv4address | reg_name ) ;
  userinfo      = ( ( unreserved | pct_encoded | sub_delims | ":" )* ) ;
  authority     = ( ( userinfo "@" )? host ( ":" port )? ) ;

  scheme        = ( alpha ( alpha | digit | "+" | "-" | "." )* ) ;

  relative_part = ( "//" authority path_abempty
                  | path_absolute
                  | path_noscheme
                  | path_empty
                  ) ;


  hier_part     = ( "//" authority path_abempty
                  | path_absolute
                  | path_rootless
                  | path_empty
                  ) ;

  absolute_URI  = ( scheme ":" hier_part ( "?" query )? ) ;

  relative_ref  = ( (relative_part %request_path ( "?" %start_query query )?) >mark %request_uri ( "#" fragment )? ) ;
  URI           = ( scheme ":" (hier_part  %request_path ( "?" %start_query query )?) >mark %request_uri ( "#" fragment )? ) ;

  URI_reference = ( URI | relative_ref ) ;

# HTTP header parsing
  Method = ( upper | digit ){1,20} >mark %request_method;

  http_number = ( "1." ("0" | "1") ) ;
  HTTP_Version = ( "HTTP/" http_number ) >mark %http_version ;
  Request_Line = ( Method " " URI_reference " " HTTP_Version CRLF ) ;

  HTTP_CTL = (0 - 31) | 127 ;
  HTTP_separator = ( "(" | ")" | "<" | ">" | "@"
                   | "," | ";" | ":" | "\\" | "\""
                   | "/" | "[" | "]" | "?" | "="
                   | "{" | "}" | " " | "\t"
                   ) ;

  lws = CRLF? (" " | "\t")+ ;
  token = ascii -- ( HTTP_CTL | HTTP_separator ) ;
  content = ((any -- HTTP_CTL) | lws);

  field_name = ( token @field_hash )+ >start_field %write_field;

  field_value = content* >start_value %write_value;

  message_header = field_name ":" lws* field_value :> CRLF;

  Request = Request_Line ( message_header )* ( CRLF );

  #SocketJSONStart = ("@" relative_part);
  #SocketJSONData = "{" any* "}" :>> "\0";

  #SocketXMLData = ("<" [a-z0-9A-Z\-.]+) >mark %request_path ("/" | space | ">") any* ">" :>> "\0";

  #SocketJSON = SocketJSONStart >mark %request_path " " SocketJSONData >mark @json;
  #SocketXML = SocketXMLData @xml;

  #SocketRequest = (SocketXML | SocketJSON);

#main := (Request | SocketRequest) @done;
#main := (Request ) @done;
# a bit more permissive
main := (CRLF)* (Request) @done;
}%%

/** Data **/

%% write data;

int http11_parser_init(http11_parser *parser) {
  int cs = 0;
  %% write init;
  parser->cs = cs;
  parser->body_start = 0;
  //parser->content_len = 0;
  parser->global_offset = 0;
  //parser->content_length= 0;
  parser->mark = 0;
  parser->field_len = 0;
  parser->field_start = 0;
  //parser->xml_sent = 0;
  //parser->json_sent = 0;

  return(1);
}


/** exec **/
uint16_t http11_parser_execute(http11_parser *parser, const char *buffer, uint16_t len, uint16_t off)
{
  if(len == 0) return 0;

  const char *p, *pe;
  int cs = parser->cs;

  assert(off <= len && "offset past end of buffer");

  p = buffer+off;
  pe = buffer+len;

  assert(pe - p == (int)len - (int)off && "pointers aren't same distance");
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
  %% write exec;
  #pragma GCC diagnostic pop

  assert(p <= pe && "Buffer overflow after parsing.");

  if (!http11_parser_has_error(parser)) {
      parser->cs = cs;
  }

  parser->global_offset += p - (buffer + off);

  assert(parser->body_start <= len && "body starts after buffer end");
  assert(parser->mark < len && "mark is after buffer end");
  assert(parser->field_len <= len && "field has length longer than whole buffer");
  assert(parser->field_start==0 || (parser->field_start <= len && "field starts after buffer end"));

  return(parser->global_offset);
}

int http11_parser_finish(http11_parser *parser)
{
  if (http11_parser_has_error(parser) ) {
    return -1;
  } else if (http11_parser_is_finished(parser) ) {
    return 1;
  } else {
    return 0;
  }
}

int http11_parser_has_error(http11_parser *parser) {
  return parser->cs == http11_parser_error;
}

int http11_parser_is_finished(http11_parser *parser) {
  return parser->cs >= http11_parser_first_final;
}
