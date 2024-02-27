
#line 1 "http11_parser.rl"
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


#line 269 "http11_parser.rl"


/** Data **/


#line 59 "http11_parser.c"
static const int http11_parser_start = 1;
static const int http11_parser_first_final = 195;
static const int http11_parser_error = 0;

static const int http11_parser_en_main = 1;


#line 274 "http11_parser.rl"

int http11_parser_init(http11_parser *parser) {
  int cs = 0;
  
#line 72 "http11_parser.c"
	{
	cs = http11_parser_start;
	}

#line 278 "http11_parser.rl"
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
  
#line 110 "http11_parser.c"
	{
	short _widec;
	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	switch ( cs ) {
case 1:
	switch( (*p) ) {
		case 10: goto tr0;
		case 13: goto tr2;
	}
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr3;
	} else if ( (*p) >= 48 )
		goto tr3;
	goto tr1;
case 0:
	goto _out;
case 2:
	if ( (*p) == 10 )
		goto tr0;
	goto tr1;
case 3:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr5;
	} else if ( (*p) >= 48 )
		goto tr5;
	goto tr1;
case 4:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr6;
		case 33: goto tr7;
		case 35: goto tr8;
		case 37: goto tr9;
		case 47: goto tr10;
		case 59: goto tr7;
		case 61: goto tr7;
		case 63: goto tr11;
		case 64: goto tr7;
		case 95: goto tr7;
		case 126: goto tr7;
		case 606: goto tr7;
		case 635: goto tr7;
		case 637: goto tr7;
	}
	if ( _widec < 65 ) {
		if ( 36 <= _widec && _widec <= 57 )
			goto tr7;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr12;
	} else
		goto tr12;
	goto tr1;
case 5:
	if ( (*p) == 72 )
		goto tr13;
	goto tr1;
case 6:
	if ( (*p) == 84 )
		goto tr14;
	goto tr1;
case 7:
	if ( (*p) == 84 )
		goto tr15;
	goto tr1;
case 8:
	if ( (*p) == 80 )
		goto tr16;
	goto tr1;
case 9:
	if ( (*p) == 47 )
		goto tr17;
	goto tr1;
case 10:
	if ( (*p) == 49 )
		goto tr18;
	goto tr1;
case 11:
	if ( (*p) == 46 )
		goto tr19;
	goto tr1;
case 12:
	if ( 48 <= (*p) && (*p) <= 49 )
		goto tr20;
	goto tr1;
case 13:
	switch( (*p) ) {
		case 10: goto tr21;
		case 13: goto tr22;
	}
	goto tr1;
case 14:
	switch( (*p) ) {
		case 10: goto tr24;
		case 13: goto tr25;
		case 33: goto tr23;
		case 124: goto tr23;
		case 126: goto tr23;
	}
	if ( (*p) < 42 ) {
		if ( (*p) < 11 ) {
			if ( 1 <= (*p) && (*p) <= 8 )
				goto tr23;
		} else if ( (*p) > 31 ) {
			if ( 35 <= (*p) && (*p) <= 39 )
				goto tr23;
		} else
			goto tr23;
	} else if ( (*p) > 43 ) {
		if ( (*p) < 48 ) {
			if ( 45 <= (*p) && (*p) <= 46 )
				goto tr23;
		} else if ( (*p) > 57 ) {
			if ( (*p) > 90 ) {
				if ( 94 <= (*p) && (*p) <= 122 )
					goto tr23;
			} else if ( (*p) >= 65 )
				goto tr23;
		} else
			goto tr23;
	} else
		goto tr23;
	goto tr1;
case 15:
	switch( (*p) ) {
		case 33: goto tr26;
		case 58: goto tr27;
		case 124: goto tr26;
		case 126: goto tr26;
	}
	if ( (*p) < 42 ) {
		if ( (*p) < 10 ) {
			if ( 1 <= (*p) && (*p) <= 8 )
				goto tr26;
		} else if ( (*p) > 31 ) {
			if ( 35 <= (*p) && (*p) <= 39 )
				goto tr26;
		} else
			goto tr26;
	} else if ( (*p) > 43 ) {
		if ( (*p) < 48 ) {
			if ( 45 <= (*p) && (*p) <= 46 )
				goto tr26;
		} else if ( (*p) > 57 ) {
			if ( (*p) > 90 ) {
				if ( 94 <= (*p) && (*p) <= 122 )
					goto tr26;
			} else if ( (*p) >= 65 )
				goto tr26;
		} else
			goto tr26;
	} else
		goto tr26;
	goto tr1;
case 16:
	switch( (*p) ) {
		case 0: goto tr1;
		case 9: goto tr29;
		case 10: goto tr30;
		case 13: goto tr31;
		case 32: goto tr29;
		case 127: goto tr1;
	}
	goto tr28;
case 17:
	switch( (*p) ) {
		case 0: goto tr1;
		case 10: goto tr33;
		case 13: goto tr34;
		case 127: goto tr1;
	}
	goto tr32;
case 18:
	if ( (*p) == 10 )
		goto tr35;
	goto tr1;
case 195:
	switch( (*p) ) {
		case 33: goto tr26;
		case 58: goto tr27;
		case 124: goto tr26;
		case 126: goto tr26;
	}
	if ( (*p) < 42 ) {
		if ( (*p) < 10 ) {
			if ( 1 <= (*p) && (*p) <= 8 )
				goto tr26;
		} else if ( (*p) > 31 ) {
			if ( 35 <= (*p) && (*p) <= 39 )
				goto tr26;
		} else
			goto tr26;
	} else if ( (*p) > 43 ) {
		if ( (*p) < 48 ) {
			if ( 45 <= (*p) && (*p) <= 46 )
				goto tr26;
		} else if ( (*p) > 57 ) {
			if ( (*p) > 90 ) {
				if ( 94 <= (*p) && (*p) <= 122 )
					goto tr26;
			} else if ( (*p) >= 65 )
				goto tr26;
		} else
			goto tr26;
	} else
		goto tr26;
	goto tr1;
case 19:
	switch( (*p) ) {
		case 10: goto tr36;
		case 33: goto tr26;
		case 58: goto tr27;
		case 124: goto tr26;
		case 126: goto tr26;
	}
	if ( (*p) < 42 ) {
		if ( (*p) < 11 ) {
			if ( 1 <= (*p) && (*p) <= 8 )
				goto tr26;
		} else if ( (*p) > 31 ) {
			if ( 35 <= (*p) && (*p) <= 39 )
				goto tr26;
		} else
			goto tr26;
	} else if ( (*p) > 43 ) {
		if ( (*p) < 48 ) {
			if ( 45 <= (*p) && (*p) <= 46 )
				goto tr26;
		} else if ( (*p) > 57 ) {
			if ( (*p) > 90 ) {
				if ( 94 <= (*p) && (*p) <= 122 )
					goto tr26;
			} else if ( (*p) >= 65 )
				goto tr26;
		} else
			goto tr26;
	} else
		goto tr26;
	goto tr1;
case 20:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr38;
		case 35: goto tr39;
		case 37: goto tr40;
		case 47: goto tr41;
		case 59: goto tr38;
		case 61: goto tr38;
		case 63: goto tr42;
		case 95: goto tr38;
		case 126: goto tr38;
		case 606: goto tr38;
		case 635: goto tr38;
		case 637: goto tr38;
	}
	if ( _widec < 64 ) {
		if ( 36 <= _widec && _widec <= 57 )
			goto tr38;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr38;
	} else
		goto tr38;
	goto tr1;
case 21:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr43;
		case 33: goto tr44;
		case 37: goto tr45;
		case 61: goto tr44;
		case 95: goto tr44;
		case 126: goto tr44;
		case 606: goto tr44;
		case 635: goto tr44;
		case 637: goto tr44;
	}
	if ( _widec < 63 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr44;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr44;
	} else
		goto tr44;
	goto tr1;
case 22:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr46;
		case 33: goto tr47;
		case 37: goto tr48;
		case 61: goto tr47;
		case 95: goto tr47;
		case 126: goto tr47;
		case 606: goto tr47;
		case 635: goto tr47;
		case 637: goto tr47;
	}
	if ( _widec < 63 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr47;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr47;
	} else
		goto tr47;
	goto tr1;
case 23:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr49;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr49;
	} else
		goto tr49;
	goto tr1;
case 24:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr47;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr47;
	} else
		goto tr47;
	goto tr1;
case 25:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr50;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr50;
	} else
		goto tr50;
	goto tr1;
case 26:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr38;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr38;
	} else
		goto tr38;
	goto tr1;
case 27:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr41;
		case 35: goto tr39;
		case 37: goto tr51;
		case 61: goto tr41;
		case 63: goto tr42;
		case 95: goto tr41;
		case 126: goto tr41;
		case 606: goto tr41;
		case 635: goto tr41;
		case 637: goto tr41;
	}
	if ( _widec < 64 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr41;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr41;
	} else
		goto tr41;
	goto tr1;
case 28:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr52;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr52;
	} else
		goto tr52;
	goto tr1;
case 29:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr41;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr41;
	} else
		goto tr41;
	goto tr1;
case 30:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr53;
		case 33: goto tr54;
		case 35: goto tr55;
		case 37: goto tr56;
		case 61: goto tr54;
		case 95: goto tr54;
		case 126: goto tr54;
		case 606: goto tr54;
		case 635: goto tr54;
		case 637: goto tr54;
	}
	if ( _widec < 63 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr54;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr54;
	} else
		goto tr54;
	goto tr1;
case 31:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr57;
		case 33: goto tr58;
		case 35: goto tr59;
		case 37: goto tr60;
		case 61: goto tr58;
		case 95: goto tr58;
		case 126: goto tr58;
		case 606: goto tr58;
		case 635: goto tr58;
		case 637: goto tr58;
	}
	if ( _widec < 63 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr58;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr58;
	} else
		goto tr58;
	goto tr1;
case 32:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr61;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr61;
	} else
		goto tr61;
	goto tr1;
case 33:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr58;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr58;
	} else
		goto tr58;
	goto tr1;
case 34:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr41;
		case 35: goto tr39;
		case 37: goto tr51;
		case 47: goto tr62;
		case 61: goto tr41;
		case 63: goto tr42;
		case 95: goto tr41;
		case 126: goto tr41;
		case 606: goto tr41;
		case 635: goto tr41;
		case 637: goto tr41;
	}
	if ( _widec < 64 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr41;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr41;
	} else
		goto tr41;
	goto tr1;
case 35:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr63;
		case 35: goto tr39;
		case 37: goto tr64;
		case 47: goto tr41;
		case 58: goto tr65;
		case 61: goto tr63;
		case 63: goto tr42;
		case 64: goto tr66;
		case 91: goto tr67;
		case 95: goto tr63;
		case 126: goto tr63;
		case 606: goto tr63;
		case 635: goto tr63;
		case 637: goto tr63;
	}
	if ( _widec < 65 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr63;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr63;
	} else
		goto tr63;
	goto tr1;
case 36:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr63;
		case 35: goto tr39;
		case 37: goto tr64;
		case 47: goto tr41;
		case 58: goto tr65;
		case 61: goto tr63;
		case 63: goto tr42;
		case 64: goto tr66;
		case 95: goto tr63;
		case 126: goto tr63;
		case 606: goto tr63;
		case 635: goto tr63;
		case 637: goto tr63;
	}
	if ( _widec < 65 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr63;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr63;
	} else
		goto tr63;
	goto tr1;
case 37:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr68;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr68;
	} else
		goto tr68;
	goto tr1;
case 38:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr63;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr63;
	} else
		goto tr63;
	goto tr1;
case 39:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr69;
		case 35: goto tr39;
		case 37: goto tr70;
		case 47: goto tr41;
		case 61: goto tr69;
		case 63: goto tr42;
		case 64: goto tr66;
		case 95: goto tr69;
		case 126: goto tr69;
		case 606: goto tr69;
		case 635: goto tr69;
		case 637: goto tr69;
	}
	if ( _widec < 58 ) {
		if ( _widec > 46 ) {
			if ( 48 <= _widec && _widec <= 57 )
				goto tr65;
		} else if ( _widec >= 36 )
			goto tr69;
	} else if ( _widec > 59 ) {
		if ( _widec > 90 ) {
			if ( 97 <= _widec && _widec <= 122 )
				goto tr69;
		} else if ( _widec >= 65 )
			goto tr69;
	} else
		goto tr69;
	goto tr1;
case 40:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 33: goto tr69;
		case 37: goto tr70;
		case 61: goto tr69;
		case 64: goto tr66;
		case 95: goto tr69;
		case 126: goto tr69;
		case 606: goto tr69;
		case 635: goto tr69;
		case 637: goto tr69;
	}
	if ( _widec < 48 ) {
		if ( 36 <= _widec && _widec <= 46 )
			goto tr69;
	} else if ( _widec > 59 ) {
		if ( _widec > 90 ) {
			if ( 97 <= _widec && _widec <= 122 )
				goto tr69;
		} else if ( _widec >= 65 )
			goto tr69;
	} else
		goto tr69;
	goto tr1;
case 41:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr71;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr71;
	} else
		goto tr71;
	goto tr1;
case 42:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr69;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr69;
	} else
		goto tr69;
	goto tr1;
case 43:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr72;
		case 35: goto tr39;
		case 37: goto tr73;
		case 47: goto tr41;
		case 58: goto tr74;
		case 61: goto tr72;
		case 63: goto tr42;
		case 91: goto tr67;
		case 95: goto tr72;
		case 126: goto tr72;
		case 606: goto tr72;
		case 635: goto tr72;
		case 637: goto tr72;
	}
	if ( _widec < 65 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr72;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr72;
	} else
		goto tr72;
	goto tr1;
case 44:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr72;
		case 35: goto tr39;
		case 37: goto tr73;
		case 47: goto tr41;
		case 58: goto tr74;
		case 61: goto tr72;
		case 63: goto tr42;
		case 95: goto tr72;
		case 126: goto tr72;
		case 606: goto tr72;
		case 635: goto tr72;
		case 637: goto tr72;
	}
	if ( _widec < 65 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr72;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr72;
	} else
		goto tr72;
	goto tr1;
case 45:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr75;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr75;
	} else
		goto tr75;
	goto tr1;
case 46:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr72;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr72;
	} else
		goto tr72;
	goto tr1;
case 47:
	switch( (*p) ) {
		case 32: goto tr37;
		case 35: goto tr39;
		case 47: goto tr41;
		case 63: goto tr42;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr74;
	goto tr1;
case 48:
	switch( (*p) ) {
		case 6: goto tr76;
		case 58: goto tr78;
		case 118: goto tr79;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr77;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr77;
	} else
		goto tr77;
	goto tr1;
case 49:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr80;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr80;
	} else
		goto tr80;
	goto tr1;
case 50:
	if ( (*p) == 58 )
		goto tr82;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr81;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr81;
	} else
		goto tr81;
	goto tr1;
case 51:
	if ( (*p) == 58 )
		goto tr82;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr83;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr83;
	} else
		goto tr83;
	goto tr1;
case 52:
	if ( (*p) == 58 )
		goto tr82;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr84;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr84;
	} else
		goto tr84;
	goto tr1;
case 53:
	if ( (*p) == 58 )
		goto tr82;
	goto tr1;
case 54:
	switch( (*p) ) {
		case 49: goto tr86;
		case 50: goto tr87;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr85;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr88;
	} else
		goto tr88;
	goto tr1;
case 55:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr90;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr90;
	} else
		goto tr90;
	goto tr1;
case 56:
	switch( (*p) ) {
		case 49: goto tr93;
		case 50: goto tr94;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr92;
	goto tr1;
case 57:
	if ( (*p) == 46 )
		goto tr95;
	goto tr1;
case 58:
	switch( (*p) ) {
		case 49: goto tr97;
		case 50: goto tr98;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr96;
	goto tr1;
case 59:
	if ( (*p) == 46 )
		goto tr99;
	goto tr1;
case 60:
	switch( (*p) ) {
		case 49: goto tr101;
		case 50: goto tr102;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr100;
	goto tr1;
case 61:
	if ( (*p) == 93 )
		goto tr103;
	goto tr1;
case 62:
	switch( (*p) ) {
		case 32: goto tr37;
		case 35: goto tr39;
		case 47: goto tr41;
		case 58: goto tr74;
		case 63: goto tr42;
	}
	goto tr1;
case 63:
	if ( (*p) == 93 )
		goto tr103;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr104;
	goto tr1;
case 64:
	if ( (*p) == 93 )
		goto tr103;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr100;
	goto tr1;
case 65:
	switch( (*p) ) {
		case 48: goto tr105;
		case 53: goto tr106;
		case 93: goto tr103;
	}
	goto tr1;
case 66:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr100;
	goto tr1;
case 67:
	if ( (*p) == 48 )
		goto tr100;
	goto tr1;
case 68:
	if ( (*p) == 46 )
		goto tr99;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr107;
	goto tr1;
case 69:
	if ( (*p) == 46 )
		goto tr99;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr96;
	goto tr1;
case 70:
	switch( (*p) ) {
		case 46: goto tr99;
		case 48: goto tr108;
		case 53: goto tr109;
	}
	goto tr1;
case 71:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr96;
	goto tr1;
case 72:
	if ( (*p) == 48 )
		goto tr96;
	goto tr1;
case 73:
	if ( (*p) == 46 )
		goto tr95;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr110;
	goto tr1;
case 74:
	if ( (*p) == 46 )
		goto tr95;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr92;
	goto tr1;
case 75:
	switch( (*p) ) {
		case 46: goto tr95;
		case 48: goto tr111;
		case 53: goto tr112;
	}
	goto tr1;
case 76:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr92;
	goto tr1;
case 77:
	if ( (*p) == 48 )
		goto tr92;
	goto tr1;
case 78:
	if ( (*p) == 58 )
		goto tr91;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr113;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr113;
	} else
		goto tr113;
	goto tr1;
case 79:
	if ( (*p) == 58 )
		goto tr91;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr114;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr114;
	} else
		goto tr114;
	goto tr1;
case 80:
	if ( (*p) == 58 )
		goto tr91;
	goto tr1;
case 81:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr115;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr115;
	} else
		goto tr115;
	goto tr1;
case 82:
	if ( (*p) == 93 )
		goto tr103;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr116;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr116;
	} else
		goto tr116;
	goto tr1;
case 83:
	if ( (*p) == 93 )
		goto tr103;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr117;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr117;
	} else
		goto tr117;
	goto tr1;
case 84:
	if ( (*p) == 93 )
		goto tr103;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr100;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr100;
	} else
		goto tr100;
	goto tr1;
case 85:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr118;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr90;
	} else
		goto tr90;
	goto tr1;
case 86:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr119;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr113;
	} else
		goto tr113;
	goto tr1;
case 87:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr114;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr114;
	} else
		goto tr114;
	goto tr1;
case 88:
	switch( (*p) ) {
		case 46: goto tr89;
		case 48: goto tr120;
		case 53: goto tr121;
		case 58: goto tr91;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr90;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr90;
	} else
		goto tr90;
	goto tr1;
case 89:
	if ( (*p) == 58 )
		goto tr91;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr119;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr113;
	} else
		goto tr113;
	goto tr1;
case 90:
	switch( (*p) ) {
		case 48: goto tr119;
		case 58: goto tr91;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr113;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr113;
	} else
		goto tr113;
	goto tr1;
case 91:
	if ( (*p) == 58 )
		goto tr91;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr90;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr90;
	} else
		goto tr90;
	goto tr1;
case 92:
	if ( (*p) == 58 )
		goto tr123;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr122;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr122;
	} else
		goto tr122;
	goto tr1;
case 93:
	if ( (*p) == 58 )
		goto tr123;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr124;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr124;
	} else
		goto tr124;
	goto tr1;
case 94:
	if ( (*p) == 58 )
		goto tr123;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr125;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr125;
	} else
		goto tr125;
	goto tr1;
case 95:
	if ( (*p) == 58 )
		goto tr123;
	goto tr1;
case 96:
	if ( (*p) == 58 )
		goto tr127;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr126;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr126;
	} else
		goto tr126;
	goto tr1;
case 97:
	if ( (*p) == 58 )
		goto tr129;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr128;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr128;
	} else
		goto tr128;
	goto tr1;
case 98:
	if ( (*p) == 58 )
		goto tr129;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr130;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr130;
	} else
		goto tr130;
	goto tr1;
case 99:
	if ( (*p) == 58 )
		goto tr129;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr131;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr131;
	} else
		goto tr131;
	goto tr1;
case 100:
	if ( (*p) == 58 )
		goto tr129;
	goto tr1;
case 101:
	if ( (*p) == 58 )
		goto tr133;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr132;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr132;
	} else
		goto tr132;
	goto tr1;
case 102:
	if ( (*p) == 58 )
		goto tr135;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr134;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr134;
	} else
		goto tr134;
	goto tr1;
case 103:
	if ( (*p) == 58 )
		goto tr135;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr136;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr136;
	} else
		goto tr136;
	goto tr1;
case 104:
	if ( (*p) == 58 )
		goto tr135;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr137;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr137;
	} else
		goto tr137;
	goto tr1;
case 105:
	if ( (*p) == 58 )
		goto tr135;
	goto tr1;
case 106:
	if ( (*p) == 58 )
		goto tr139;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr138;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr138;
	} else
		goto tr138;
	goto tr1;
case 107:
	if ( (*p) == 58 )
		goto tr141;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr140;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr140;
	} else
		goto tr140;
	goto tr1;
case 108:
	if ( (*p) == 58 )
		goto tr141;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr142;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr142;
	} else
		goto tr142;
	goto tr1;
case 109:
	if ( (*p) == 58 )
		goto tr141;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr143;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr143;
	} else
		goto tr143;
	goto tr1;
case 110:
	if ( (*p) == 58 )
		goto tr141;
	goto tr1;
case 111:
	if ( (*p) == 58 )
		goto tr145;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr144;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr144;
	} else
		goto tr144;
	goto tr1;
case 112:
	if ( (*p) == 58 )
		goto tr147;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr146;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr146;
	} else
		goto tr146;
	goto tr1;
case 113:
	if ( (*p) == 58 )
		goto tr147;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr148;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr148;
	} else
		goto tr148;
	goto tr1;
case 114:
	if ( (*p) == 58 )
		goto tr147;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr149;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr149;
	} else
		goto tr149;
	goto tr1;
case 115:
	if ( (*p) == 58 )
		goto tr147;
	goto tr1;
case 116:
	if ( (*p) == 58 )
		goto tr151;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr150;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr150;
	} else
		goto tr150;
	goto tr1;
case 117:
	if ( (*p) == 58 )
		goto tr153;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr152;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr152;
	} else
		goto tr152;
	goto tr1;
case 118:
	if ( (*p) == 58 )
		goto tr153;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr154;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr154;
	} else
		goto tr154;
	goto tr1;
case 119:
	if ( (*p) == 58 )
		goto tr153;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr155;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr155;
	} else
		goto tr155;
	goto tr1;
case 120:
	if ( (*p) == 58 )
		goto tr153;
	goto tr1;
case 121:
	if ( (*p) == 58 )
		goto tr157;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr156;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr156;
	} else
		goto tr156;
	goto tr1;
case 122:
	if ( (*p) == 58 )
		goto tr159;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr158;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr158;
	} else
		goto tr158;
	goto tr1;
case 123:
	if ( (*p) == 58 )
		goto tr159;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr160;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr160;
	} else
		goto tr160;
	goto tr1;
case 124:
	if ( (*p) == 58 )
		goto tr159;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr161;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr161;
	} else
		goto tr161;
	goto tr1;
case 125:
	if ( (*p) == 58 )
		goto tr159;
	goto tr1;
case 126:
	if ( (*p) == 58 )
		goto tr162;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr156;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr156;
	} else
		goto tr156;
	goto tr1;
case 127:
	switch( (*p) ) {
		case 49: goto tr164;
		case 50: goto tr165;
		case 93: goto tr103;
	}
	if ( (*p) < 48 ) {
		if ( 2 <= (*p) && (*p) <= 3 )
			goto tr76;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 70 ) {
			if ( 97 <= (*p) && (*p) <= 102 )
				goto tr166;
		} else if ( (*p) >= 65 )
			goto tr166;
	} else
		goto tr163;
	goto tr1;
case 128:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr167;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr167;
	} else
		goto tr167;
	goto tr1;
case 129:
	switch( (*p) ) {
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr169;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr169;
	} else
		goto tr169;
	goto tr1;
case 130:
	switch( (*p) ) {
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr170;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr170;
	} else
		goto tr170;
	goto tr1;
case 131:
	switch( (*p) ) {
		case 58: goto tr168;
		case 93: goto tr103;
	}
	goto tr1;
case 132:
	switch( (*p) ) {
		case 49: goto tr172;
		case 50: goto tr173;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr171;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr174;
	} else
		goto tr174;
	goto tr1;
case 133:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr175;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr175;
	} else
		goto tr175;
	goto tr1;
case 134:
	switch( (*p) ) {
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr176;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr176;
	} else
		goto tr176;
	goto tr1;
case 135:
	switch( (*p) ) {
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr177;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr177;
	} else
		goto tr177;
	goto tr1;
case 136:
	switch( (*p) ) {
		case 58: goto tr91;
		case 93: goto tr103;
	}
	goto tr1;
case 137:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr178;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr175;
	} else
		goto tr175;
	goto tr1;
case 138:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr179;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr176;
	} else
		goto tr176;
	goto tr1;
case 139:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr177;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr177;
	} else
		goto tr177;
	goto tr1;
case 140:
	switch( (*p) ) {
		case 46: goto tr89;
		case 48: goto tr180;
		case 53: goto tr181;
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr175;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr175;
	} else
		goto tr175;
	goto tr1;
case 141:
	switch( (*p) ) {
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr179;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr176;
	} else
		goto tr176;
	goto tr1;
case 142:
	switch( (*p) ) {
		case 48: goto tr179;
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr176;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr176;
	} else
		goto tr176;
	goto tr1;
case 143:
	switch( (*p) ) {
		case 58: goto tr91;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr175;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr175;
	} else
		goto tr175;
	goto tr1;
case 144:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr182;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr167;
	} else
		goto tr167;
	goto tr1;
case 145:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr183;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr169;
	} else
		goto tr169;
	goto tr1;
case 146:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr170;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr170;
	} else
		goto tr170;
	goto tr1;
case 147:
	switch( (*p) ) {
		case 46: goto tr89;
		case 48: goto tr184;
		case 53: goto tr185;
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr167;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr167;
	} else
		goto tr167;
	goto tr1;
case 148:
	switch( (*p) ) {
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr183;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr169;
	} else
		goto tr169;
	goto tr1;
case 149:
	switch( (*p) ) {
		case 48: goto tr183;
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr169;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr169;
	} else
		goto tr169;
	goto tr1;
case 150:
	switch( (*p) ) {
		case 58: goto tr168;
		case 93: goto tr103;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr167;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr167;
	} else
		goto tr167;
	goto tr1;
case 151:
	switch( (*p) ) {
		case 49: goto tr164;
		case 50: goto tr165;
	}
	if ( (*p) < 48 ) {
		if ( 2 <= (*p) && (*p) <= 3 )
			goto tr76;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 70 ) {
			if ( 97 <= (*p) && (*p) <= 102 )
				goto tr166;
		} else if ( (*p) >= 65 )
			goto tr166;
	} else
		goto tr163;
	goto tr1;
case 152:
	switch( (*p) ) {
		case 49: goto tr187;
		case 50: goto tr188;
	}
	if ( (*p) < 48 ) {
		if ( 2 <= (*p) && (*p) <= 3 )
			goto tr76;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 70 ) {
			if ( 97 <= (*p) && (*p) <= 102 )
				goto tr189;
		} else if ( (*p) >= 65 )
			goto tr189;
	} else
		goto tr186;
	goto tr1;
case 153:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr190;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr190;
	} else
		goto tr190;
	goto tr1;
case 154:
	if ( (*p) == 58 )
		goto tr168;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr191;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr191;
	} else
		goto tr191;
	goto tr1;
case 155:
	if ( (*p) == 58 )
		goto tr168;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr192;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr192;
	} else
		goto tr192;
	goto tr1;
case 156:
	if ( (*p) == 58 )
		goto tr168;
	goto tr1;
case 157:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr193;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr190;
	} else
		goto tr190;
	goto tr1;
case 158:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr194;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr191;
	} else
		goto tr191;
	goto tr1;
case 159:
	switch( (*p) ) {
		case 46: goto tr89;
		case 58: goto tr168;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr192;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr192;
	} else
		goto tr192;
	goto tr1;
case 160:
	switch( (*p) ) {
		case 46: goto tr89;
		case 48: goto tr195;
		case 53: goto tr196;
		case 58: goto tr168;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr190;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr190;
	} else
		goto tr190;
	goto tr1;
case 161:
	if ( (*p) == 58 )
		goto tr168;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr194;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr191;
	} else
		goto tr191;
	goto tr1;
case 162:
	switch( (*p) ) {
		case 48: goto tr194;
		case 58: goto tr168;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr191;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr191;
	} else
		goto tr191;
	goto tr1;
case 163:
	if ( (*p) == 58 )
		goto tr168;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr190;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr190;
	} else
		goto tr190;
	goto tr1;
case 164:
	if ( (*p) < 48 ) {
		if ( 2 <= (*p) && (*p) <= 3 )
			goto tr76;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 70 ) {
			if ( 97 <= (*p) && (*p) <= 102 )
				goto tr80;
		} else if ( (*p) >= 65 )
			goto tr80;
	} else
		goto tr80;
	goto tr1;
case 165:
	if ( 2 <= (*p) && (*p) <= 3 )
		goto tr76;
	goto tr1;
case 166:
	if ( (*p) == 3 )
		goto tr76;
	goto tr1;
case 167:
	if ( (*p) == 4 )
		goto tr76;
	goto tr1;
case 168:
	if ( (*p) == 58 )
		goto tr197;
	goto tr1;
case 169:
	switch( (*p) ) {
		case 49: goto tr164;
		case 50: goto tr165;
		case 93: goto tr103;
	}
	if ( (*p) < 48 ) {
		if ( 2 <= (*p) && (*p) <= 5 )
			goto tr76;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 70 ) {
			if ( 97 <= (*p) && (*p) <= 102 )
				goto tr166;
		} else if ( (*p) >= 65 )
			goto tr166;
	} else
		goto tr163;
	goto tr1;
case 170:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr198;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr198;
	} else
		goto tr198;
	goto tr1;
case 171:
	if ( (*p) == 46 )
		goto tr199;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr198;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr198;
	} else
		goto tr198;
	goto tr1;
case 172:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 33: goto tr200;
		case 36: goto tr200;
		case 61: goto tr200;
		case 95: goto tr200;
		case 126: goto tr200;
		case 606: goto tr200;
		case 635: goto tr200;
		case 637: goto tr200;
	}
	if ( _widec < 48 ) {
		if ( 38 <= _widec && _widec <= 46 )
			goto tr200;
	} else if ( _widec > 59 ) {
		if ( _widec > 90 ) {
			if ( 97 <= _widec && _widec <= 122 )
				goto tr200;
		} else if ( _widec >= 65 )
			goto tr200;
	} else
		goto tr200;
	goto tr1;
case 173:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 33: goto tr200;
		case 36: goto tr200;
		case 61: goto tr200;
		case 93: goto tr103;
		case 95: goto tr200;
		case 126: goto tr200;
		case 606: goto tr200;
		case 635: goto tr200;
		case 637: goto tr200;
	}
	if ( _widec < 48 ) {
		if ( 38 <= _widec && _widec <= 46 )
			goto tr200;
	} else if ( _widec > 59 ) {
		if ( _widec > 90 ) {
			if ( 97 <= _widec && _widec <= 122 )
				goto tr200;
		} else if ( _widec >= 65 )
			goto tr200;
	} else
		goto tr200;
	goto tr1;
case 174:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr37;
		case 33: goto tr38;
		case 35: goto tr39;
		case 37: goto tr40;
		case 43: goto tr201;
		case 47: goto tr41;
		case 58: goto tr202;
		case 59: goto tr38;
		case 61: goto tr38;
		case 63: goto tr42;
		case 64: goto tr38;
		case 95: goto tr38;
		case 126: goto tr38;
		case 606: goto tr38;
		case 635: goto tr38;
		case 637: goto tr38;
	}
	if ( _widec < 45 ) {
		if ( 36 <= _widec && _widec <= 44 )
			goto tr38;
	} else if ( _widec > 57 ) {
		if ( _widec > 90 ) {
			if ( 97 <= _widec && _widec <= 122 )
				goto tr201;
		} else if ( _widec >= 65 )
			goto tr201;
	} else
		goto tr201;
	goto tr1;
case 175:
	_widec = (*p);
	if ( (*p) < 123 ) {
		if ( 94 <= (*p) && (*p) <= 94 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else if ( (*p) > 123 ) {
		if ( 125 <= (*p) && (*p) <= 125 ) {
			_widec = (short)(128 + ((*p) - -128));
			if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
		}
	} else {
		_widec = (short)(128 + ((*p) - -128));
		if ( 
#line 141 "http11_parser.rl"
 parser->uri_relaxed  ) _widec += 256;
	}
	switch( _widec ) {
		case 32: goto tr6;
		case 33: goto tr203;
		case 35: goto tr8;
		case 37: goto tr204;
		case 47: goto tr10;
		case 61: goto tr203;
		case 63: goto tr11;
		case 95: goto tr203;
		case 126: goto tr203;
		case 606: goto tr203;
		case 635: goto tr203;
		case 637: goto tr203;
	}
	if ( _widec < 64 ) {
		if ( 36 <= _widec && _widec <= 59 )
			goto tr203;
	} else if ( _widec > 90 ) {
		if ( 97 <= _widec && _widec <= 122 )
			goto tr203;
	} else
		goto tr203;
	goto tr1;
case 176:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr205;
	} else if ( (*p) >= 48 )
		goto tr205;
	goto tr1;
case 177:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr206;
	} else if ( (*p) >= 48 )
		goto tr206;
	goto tr1;
case 178:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr207;
	} else if ( (*p) >= 48 )
		goto tr207;
	goto tr1;
case 179:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr208;
	} else if ( (*p) >= 48 )
		goto tr208;
	goto tr1;
case 180:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr209;
	} else if ( (*p) >= 48 )
		goto tr209;
	goto tr1;
case 181:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr210;
	} else if ( (*p) >= 48 )
		goto tr210;
	goto tr1;
case 182:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr211;
	} else if ( (*p) >= 48 )
		goto tr211;
	goto tr1;
case 183:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr212;
	} else if ( (*p) >= 48 )
		goto tr212;
	goto tr1;
case 184:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr213;
	} else if ( (*p) >= 48 )
		goto tr213;
	goto tr1;
case 185:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr214;
	} else if ( (*p) >= 48 )
		goto tr214;
	goto tr1;
case 186:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr215;
	} else if ( (*p) >= 48 )
		goto tr215;
	goto tr1;
case 187:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr216;
	} else if ( (*p) >= 48 )
		goto tr216;
	goto tr1;
case 188:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr217;
	} else if ( (*p) >= 48 )
		goto tr217;
	goto tr1;
case 189:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr218;
	} else if ( (*p) >= 48 )
		goto tr218;
	goto tr1;
case 190:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr219;
	} else if ( (*p) >= 48 )
		goto tr219;
	goto tr1;
case 191:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr220;
	} else if ( (*p) >= 48 )
		goto tr220;
	goto tr1;
case 192:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr221;
	} else if ( (*p) >= 48 )
		goto tr221;
	goto tr1;
case 193:
	if ( (*p) == 32 )
		goto tr4;
	if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr222;
	} else if ( (*p) >= 48 )
		goto tr222;
	goto tr1;
case 194:
	if ( (*p) == 32 )
		goto tr4;
	goto tr1;
	}

	tr1: cs = 0; goto _again;
	tr0: cs = 1; goto _again;
	tr2: cs = 2; goto _again;
	tr3: cs = 3; goto f0;
	tr4: cs = 4; goto f1;
	tr6: cs = 5; goto f2;
	tr37: cs = 5; goto f13;
	tr43: cs = 5; goto f15;
	tr46: cs = 5; goto f16;
	tr53: cs = 5; goto f17;
	tr57: cs = 5; goto f19;
	tr13: cs = 6; goto f0;
	tr14: cs = 7; goto _again;
	tr15: cs = 8; goto _again;
	tr16: cs = 9; goto _again;
	tr17: cs = 10; goto _again;
	tr18: cs = 11; goto _again;
	tr19: cs = 12; goto _again;
	tr20: cs = 13; goto _again;
	tr35: cs = 14; goto _again;
	tr21: cs = 14; goto f4;
	tr30: cs = 14; goto f10;
	tr33: cs = 14; goto f11;
	tr23: cs = 15; goto f5;
	tr26: cs = 15; goto f7;
	tr27: cs = 16; goto f8;
	tr29: cs = 16; goto f9;
	tr32: cs = 17; goto _again;
	tr28: cs = 17; goto f9;
	tr22: cs = 18; goto f4;
	tr31: cs = 18; goto f10;
	tr34: cs = 18; goto f11;
	tr25: cs = 19; goto f5;
	tr38: cs = 20; goto _again;
	tr7: cs = 20; goto f0;
	tr8: cs = 21; goto f2;
	tr39: cs = 21; goto f13;
	tr55: cs = 21; goto f17;
	tr59: cs = 21; goto f19;
	tr47: cs = 22; goto _again;
	tr44: cs = 22; goto f0;
	tr48: cs = 23; goto _again;
	tr45: cs = 23; goto f0;
	tr49: cs = 24; goto _again;
	tr40: cs = 25; goto _again;
	tr9: cs = 25; goto f0;
	tr50: cs = 26; goto _again;
	tr41: cs = 27; goto _again;
	tr203: cs = 27; goto f0;
	tr51: cs = 28; goto _again;
	tr204: cs = 28; goto f0;
	tr52: cs = 29; goto _again;
	tr11: cs = 30; goto f3;
	tr42: cs = 30; goto f14;
	tr58: cs = 31; goto _again;
	tr54: cs = 31; goto f18;
	tr60: cs = 32; goto _again;
	tr56: cs = 32; goto f18;
	tr61: cs = 33; goto _again;
	tr10: cs = 34; goto f0;
	tr62: cs = 35; goto _again;
	tr63: cs = 36; goto _again;
	tr64: cs = 37; goto _again;
	tr68: cs = 38; goto _again;
	tr65: cs = 39; goto _again;
	tr69: cs = 40; goto _again;
	tr70: cs = 41; goto _again;
	tr71: cs = 42; goto _again;
	tr66: cs = 43; goto _again;
	tr72: cs = 44; goto _again;
	tr73: cs = 45; goto _again;
	tr75: cs = 46; goto _again;
	tr74: cs = 47; goto _again;
	tr67: cs = 48; goto _again;
	tr76: cs = 49; goto _again;
	tr80: cs = 50; goto _again;
	tr81: cs = 51; goto _again;
	tr83: cs = 52; goto _again;
	tr84: cs = 53; goto _again;
	tr82: cs = 54; goto _again;
	tr85: cs = 55; goto _again;
	tr89: cs = 56; goto _again;
	tr92: cs = 57; goto _again;
	tr95: cs = 58; goto _again;
	tr96: cs = 59; goto _again;
	tr99: cs = 60; goto _again;
	tr100: cs = 61; goto _again;
	tr103: cs = 62; goto _again;
	tr101: cs = 63; goto _again;
	tr104: cs = 64; goto _again;
	tr102: cs = 65; goto _again;
	tr105: cs = 66; goto _again;
	tr106: cs = 67; goto _again;
	tr97: cs = 68; goto _again;
	tr107: cs = 69; goto _again;
	tr98: cs = 70; goto _again;
	tr108: cs = 71; goto _again;
	tr109: cs = 72; goto _again;
	tr93: cs = 73; goto _again;
	tr110: cs = 74; goto _again;
	tr94: cs = 75; goto _again;
	tr111: cs = 76; goto _again;
	tr112: cs = 77; goto _again;
	tr90: cs = 78; goto _again;
	tr113: cs = 79; goto _again;
	tr114: cs = 80; goto _again;
	tr91: cs = 81; goto _again;
	tr115: cs = 82; goto _again;
	tr116: cs = 83; goto _again;
	tr117: cs = 84; goto _again;
	tr86: cs = 85; goto _again;
	tr118: cs = 86; goto _again;
	tr119: cs = 87; goto _again;
	tr87: cs = 88; goto _again;
	tr120: cs = 89; goto _again;
	tr121: cs = 90; goto _again;
	tr88: cs = 91; goto _again;
	tr77: cs = 92; goto _again;
	tr122: cs = 93; goto _again;
	tr124: cs = 94; goto _again;
	tr125: cs = 95; goto _again;
	tr123: cs = 96; goto _again;
	tr126: cs = 97; goto _again;
	tr128: cs = 98; goto _again;
	tr130: cs = 99; goto _again;
	tr131: cs = 100; goto _again;
	tr129: cs = 101; goto _again;
	tr132: cs = 102; goto _again;
	tr134: cs = 103; goto _again;
	tr136: cs = 104; goto _again;
	tr137: cs = 105; goto _again;
	tr135: cs = 106; goto _again;
	tr138: cs = 107; goto _again;
	tr140: cs = 108; goto _again;
	tr142: cs = 109; goto _again;
	tr143: cs = 110; goto _again;
	tr141: cs = 111; goto _again;
	tr144: cs = 112; goto _again;
	tr146: cs = 113; goto _again;
	tr148: cs = 114; goto _again;
	tr149: cs = 115; goto _again;
	tr147: cs = 116; goto _again;
	tr150: cs = 117; goto _again;
	tr152: cs = 118; goto _again;
	tr154: cs = 119; goto _again;
	tr155: cs = 120; goto _again;
	tr153: cs = 121; goto _again;
	tr156: cs = 122; goto _again;
	tr158: cs = 123; goto _again;
	tr160: cs = 124; goto _again;
	tr161: cs = 125; goto _again;
	tr159: cs = 126; goto _again;
	tr162: cs = 127; goto _again;
	tr163: cs = 128; goto _again;
	tr167: cs = 129; goto _again;
	tr169: cs = 130; goto _again;
	tr170: cs = 131; goto _again;
	tr168: cs = 132; goto _again;
	tr171: cs = 133; goto _again;
	tr175: cs = 134; goto _again;
	tr176: cs = 135; goto _again;
	tr177: cs = 136; goto _again;
	tr172: cs = 137; goto _again;
	tr178: cs = 138; goto _again;
	tr179: cs = 139; goto _again;
	tr173: cs = 140; goto _again;
	tr180: cs = 141; goto _again;
	tr181: cs = 142; goto _again;
	tr174: cs = 143; goto _again;
	tr164: cs = 144; goto _again;
	tr182: cs = 145; goto _again;
	tr183: cs = 146; goto _again;
	tr165: cs = 147; goto _again;
	tr184: cs = 148; goto _again;
	tr185: cs = 149; goto _again;
	tr166: cs = 150; goto _again;
	tr157: cs = 151; goto _again;
	tr151: cs = 152; goto _again;
	tr186: cs = 153; goto _again;
	tr190: cs = 154; goto _again;
	tr191: cs = 155; goto _again;
	tr192: cs = 156; goto _again;
	tr187: cs = 157; goto _again;
	tr193: cs = 158; goto _again;
	tr194: cs = 159; goto _again;
	tr188: cs = 160; goto _again;
	tr195: cs = 161; goto _again;
	tr196: cs = 162; goto _again;
	tr189: cs = 163; goto _again;
	tr145: cs = 164; goto _again;
	tr139: cs = 165; goto _again;
	tr133: cs = 166; goto _again;
	tr127: cs = 167; goto _again;
	tr78: cs = 168; goto _again;
	tr197: cs = 169; goto _again;
	tr79: cs = 170; goto _again;
	tr198: cs = 171; goto _again;
	tr199: cs = 172; goto _again;
	tr200: cs = 173; goto _again;
	tr201: cs = 174; goto _again;
	tr12: cs = 174; goto f0;
	tr202: cs = 175; goto _again;
	tr5: cs = 176; goto _again;
	tr205: cs = 177; goto _again;
	tr206: cs = 178; goto _again;
	tr207: cs = 179; goto _again;
	tr208: cs = 180; goto _again;
	tr209: cs = 181; goto _again;
	tr210: cs = 182; goto _again;
	tr211: cs = 183; goto _again;
	tr212: cs = 184; goto _again;
	tr213: cs = 185; goto _again;
	tr214: cs = 186; goto _again;
	tr215: cs = 187; goto _again;
	tr216: cs = 188; goto _again;
	tr217: cs = 189; goto _again;
	tr218: cs = 190; goto _again;
	tr219: cs = 191; goto _again;
	tr220: cs = 192; goto _again;
	tr221: cs = 193; goto _again;
	tr222: cs = 194; goto _again;
	tr24: cs = 195; goto f6;
	tr36: cs = 195; goto f12;

f0:
#line 52 "http11_parser.rl"
	{MARK(mark, p); }
	goto _again;
f7:
#line 58 "http11_parser.rl"
	{ 
      parser->field_hash=((parser->field_hash << 5) + parser->field_hash) + (uint32_t) tolower((int)(*p)); 
  }
	goto _again;
f8:
#line 61 "http11_parser.rl"
	{ 
    parser->field_len = LEN(field_start, p);
  }
	goto _again;
f9:
#line 71 "http11_parser.rl"
	{ MARK(mark, p); }
	goto _again;
f11:
#line 73 "http11_parser.rl"
	{
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, 
        GET_MARK(field_hash) ,GET_MARK(field_start), parser->field_len, 
        GET_MARK(mark), LEN(mark, p));
    }
  }
	goto _again;
f1:
#line 81 "http11_parser.rl"
	{ 
    if(parser->request_method != NULL) 
      parser->request_method(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f16:
#line 91 "http11_parser.rl"
	{
    if(parser->fragment != NULL)
      parser->fragment(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f18:
#line 96 "http11_parser.rl"
	{MARK(query_start, p); }
	goto _again;
f4:
#line 102 "http11_parser.rl"
	{	
    if(parser->http_version != NULL)
      parser->http_version(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f14:
#line 107 "http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, GET_MARK(mark), LEN(mark,p));
  }
	goto _again;
f15:
#line 52 "http11_parser.rl"
	{MARK(mark, p); }
#line 91 "http11_parser.rl"
	{
    if(parser->fragment != NULL)
      parser->fragment(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f3:
#line 52 "http11_parser.rl"
	{MARK(mark, p); }
#line 107 "http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, GET_MARK(mark), LEN(mark,p));
  }
	goto _again;
f5:
#line 55 "http11_parser.rl"
	{ MARK(field_start, p); parser->field_hash=5381; /*init djb2 hash*/ }
#line 58 "http11_parser.rl"
	{ 
      parser->field_hash=((parser->field_hash << 5) + parser->field_hash) + (uint32_t) tolower((int)(*p)); 
  }
	goto _again;
f12:
#line 58 "http11_parser.rl"
	{ 
      parser->field_hash=((parser->field_hash << 5) + parser->field_hash) + (uint32_t) tolower((int)(*p)); 
  }
#line 112 "http11_parser.rl"
	{
      /* if(parser->xml_sent || parser->json_sent) { */
      /*   parser->body_start = GET_MARK(mark); */
      /*   // +1 includes the \0 */
      /*   parser->content_len = OFFSET(fpc) - parser->body_start + 1; */
      /* } else { */
        parser->body_start = OFFSET(p)+ 1;

        if(parser->header_done != NULL) {
          parser->header_done(parser->data, OFFSET(p + 1), OFFSET(pe) - OFFSET(p) - 1);
        }
      /* } */
    {p++; goto _out; }
  }
	goto _again;
f10:
#line 71 "http11_parser.rl"
	{ MARK(mark, p); }
#line 73 "http11_parser.rl"
	{
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, 
        GET_MARK(field_hash) ,GET_MARK(field_start), parser->field_len, 
        GET_MARK(mark), LEN(mark, p));
    }
  }
	goto _again;
f19:
#line 97 "http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, GET_MARK(query_start), LEN(query_start, p));
  }
#line 86 "http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f13:
#line 107 "http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, GET_MARK(mark), LEN(mark,p));
  }
#line 86 "http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f2:
#line 52 "http11_parser.rl"
	{MARK(mark, p); }
#line 107 "http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, GET_MARK(mark), LEN(mark,p));
  }
#line 86 "http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;
f6:
#line 55 "http11_parser.rl"
	{ MARK(field_start, p); parser->field_hash=5381; /*init djb2 hash*/ }
#line 58 "http11_parser.rl"
	{ 
      parser->field_hash=((parser->field_hash << 5) + parser->field_hash) + (uint32_t) tolower((int)(*p)); 
  }
#line 112 "http11_parser.rl"
	{
      /* if(parser->xml_sent || parser->json_sent) { */
      /*   parser->body_start = GET_MARK(mark); */
      /*   // +1 includes the \0 */
      /*   parser->content_len = OFFSET(fpc) - parser->body_start + 1; */
      /* } else { */
        parser->body_start = OFFSET(p)+ 1;

        if(parser->header_done != NULL) {
          parser->header_done(parser->data, OFFSET(p + 1), OFFSET(pe) - OFFSET(p) - 1);
        }
      /* } */
    {p++; goto _out; }
  }
	goto _again;
f17:
#line 96 "http11_parser.rl"
	{MARK(query_start, p); }
#line 97 "http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, GET_MARK(query_start), LEN(query_start, p));
  }
#line 86 "http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, GET_MARK(mark), LEN(mark, p));
  }
	goto _again;

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 310 "http11_parser.rl"
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
