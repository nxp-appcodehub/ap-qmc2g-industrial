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

/**
 * @brief return the status code string for an HTTP status code
 *
 * @param code HTTP status code
 *
 * @return  status string or NULL
 */
const char* status_code_string(int code)
{
	const char *result=NULL;
    switch(code){
        case 100: result="Continue"; break;
        case 101: result="Switching Protocols"; break;
        case 102: result="Processing"; break;
        case 200: result="OK"; break;
        case 201: result="Created"; break;
        case 202: result="Accepted"; break;
        case 203: result="Non-Authoritative Information"; break;
        case 204: result="No Content"; break;
        case 205: result="Reset Content"; break;
        case 206: result="Partial Content"; break;
        case 207: result="Multi-Status"; break;
        case 208: result="Already Reported"; break;
        case 226: result="IM Used"; break;
        case 300: result="Multiple Choices"; break;
        case 301: result="Moved Permanently"; break;
        case 302: result="Found"; break;
        case 303: result="See Other"; break;
        case 304: result="Not Modified"; break;
        case 305: result="Use Proxy"; break;
        case 306: result="Switch Proxy"; break;
        case 307: result="Temporary Redirect"; break;
        case 308: result="Permanent Redirect"; break;
        case 400: result="Bad Request"; break;
        case 401: result="Unauthorized"; break;
        case 402: result="Payment Required"; break;
        case 403: result="Forbidden"; break;
        case 404: result="Not Found"; break;
        case 405: result="Method Not Allowed"; break;
        case 406: result="Not Acceptable"; break;
        case 407: result="Proxy Authentication Required"; break;
        case 408: result="Request Timeout"; break;
        case 409: result="Conflict"; break;
        case 410: result="Gone"; break;
        case 411: result="Length Required"; break;
        case 412: result="Precondition Failed"; break;
        case 413: result="Payload Too Large"; break;
        case 414: result="URI Too Long"; break;
        case 415: result="Unsupported Media Type"; break;
        case 416: result="Range Not Satisfiable"; break;
        case 417: result="Expectation Failed"; break;
        case 421: result="Misdirected Request"; break;
        case 422: result="Unprocessable Entity"; break;
        case 423: result="Locked"; break;
        case 424: result="Failed Dependency"; break;
        case 426: result="Upgrade Required"; break;
        case 428: result="Precondition Required"; break;
        case 429: result="Too Many Requests"; break;
        case 431: result="Request Header Fields Too Large"; break;
        case 451: result="Unavailable For Legal Reasons"; break;
        case 500: result="Internal Server Error"; break;
        case 501: result="Not Implemented"; break;
        case 502: result="Bad Gateway"; break;
        case 503: result="Service Unavailable"; break;
        case 504: result="Gateway Timeout"; break;
        case 505: result="HTTP Version Not Supported"; break;
        case 506: result="Variant Also Negotiates"; break;
        case 507: result="Insufficient Storage"; break;
        case 508: result="Loop Detected"; break;
        case 510: result="Not Extended"; break;
        case 511: result="Network Authentication Required"; break;
        default:  break;
    }
    return result;
}
