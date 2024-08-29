/**
 ******************************************************************************
 * @file    app_httpd.h
 * @author  QQ DING
 * @version V1.0.0
 * @date    1-September-2015
 * @brief   This header contains function prototypes called by httpd protocol
 *          operations
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */
#include "http_server/web_log.h"

#define HTTP_CONTENT_HTML_ZIP "text/html\r\nContent-Encoding: gzip\r\nCache-Control: public"
#define HTTP_CONTENT_JS_ZIP "text/javascript\r\nContent-Encoding: gzip\r\nCache-Control: public"
#define HTTP_CONTENT_CSS_ZIP "text/css\r\nContent-Encoding: gzip\r\nCache-Control: public"
#define HTTP_CONTENT_WOFF2 "font/woff2\r\nCache-Control: public"

#define HTTPD_HDR_DEFORT (HTTPD_HDR_ADD_SERVER | HTTPD_HDR_ADD_CONN_CLOSE | HTTPD_HDR_ADD_PRAGMA_NO_CACHE)
#define HTTPD_HDR_CACHE (HTTPD_HDR_ADD_SERVER | HTTPD_HDR_ADD_CONN_CLOSE | HTTPD_HDR_ADD_CACHE_CTRL_NO_CHK)

#define send_http(DATA, LEN, LABEL, P_ERR)                                                       \
    *(P_ERR) = httpd_send_all_header(req, HTTP_RES_200, LEN, HTTP_CONTENT_PLAIN_TEXT_STR);             \
    require_noerr_action(*(P_ERR), LABEL, http_log("ERROR: Unable to send http DATA headers.")); \
    *(P_ERR) = httpd_send_body(req->sock, (const unsigned char *)DATA, LEN);                     \
    require_noerr_action(*(P_ERR), LABEL, http_log("ERROR: Unable to send http DATA body."));

#define send_http_chunked_header(LABEL, P_ERR)                                     \
    *(P_ERR) = http_send_chunked_header(req, HTTP_RES_200, HTTP_CONTENT_PLAIN_TEXT_STR); \
    require_noerr_action(*(P_ERR), LABEL, http_log("ERROR: Unable to send http Chunked DATA headers."));

#define send_http_chunked_data(DATA, LEN, LABEL, P_ERR) \
    *(P_ERR) = httpd_send_chunk(req->sock, DATA, LEN);  \
    require_noerr_action(*(P_ERR), LABEL, http_log("ERROR: Unable to send http Chunked DATA body."));

#define TC1_STATUS_JSON \
    "{\
    'sockets':'%s',\
    'mode':%d,\
    'station_ssid':'%s',\
    'station_pwd':'%s',\
    'ap_ssid':'%s',\
    'ap_pwd':'%s',\
    'mqtt_addr':'%s',\
    'mqtt_port':%d,\
    'mqtt_usr':'%s',\
    'mqtt_pwd':'%s',\
    'version':'%s',\
    'ip':'%s',\
    'mask':'%s',\
    'gateway':'%s',\
    'up_time':'%s' \
}"

// #define POWER_INFO_JSON "{'sockets':'%s','idx':%d,'len':%d,'p_count':%ld,'up_time':'%s','powers':[%s]}"

int AppHttpdStart(void);
int AppHttpdStop();
