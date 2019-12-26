/**
 ******************************************************************************
 * @file    app_https.c
 * @author  QQ DING
 * @version V1.0.0
 * @date    1-September-2015
 * @brief   The main HTTPD server initialization and wsgi handle.
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

#include <time.h>
#include <httpd.h>
#include <http_parse.h>
#include <http-strings.h>

#include "mico.h"
#include "httpd_priv.h"
#include "app_httpd.h"
#include "user_gpio.h"
#include "user_wifi.h"
#include "user_power.h"
#include "main.h"
#include "web_data.c"
#include "timed_task/timed_task.h"

static bool is_http_init;
static bool is_handlers_registered;
struct httpd_wsgi_call g_app_handlers[];
char power_info_json[1536] = { 0 };
char up_time[16] = "00:00:00";

static int HttpGetIndexPage(httpd_request_t *req)
{
    OSStatus err = kNoErr;

    err = httpd_send_all_header(req, HTTP_RES_200, sizeof(index_html), HTTP_CONTENT_HTML_ZIP);
    require_noerr_action(err, exit, app_httpd_log("ERROR: Unable to send http wifisetting headers."));

    err = httpd_send_body(req->sock, index_html, sizeof(index_html));
    require_noerr_action(err, exit, app_httpd_log("ERROR: Unable to send http wifisetting body."));

exit:
    return err;
}

static int HttpGetTc1Status(httpd_request_t *req)
{
    const unsigned char* sockets = GetSocketStatus();
    char* tc1_status = malloc(384);
    sprintf(tc1_status, TC1_STATUS_JSON, sockets, ip_status.mode,
        sys_config->micoSystemConfig.ssid, sys_config->micoSystemConfig.user_key,
        ELAND_AP_SSID, ELAND_AP_KEY, "MQTT.ADDR", 1883, ip_status.ip, ip_status.mask, ip_status.gateway, 0L);

    OSStatus err = kNoErr;
    send_http(tc1_status, strlen(tc1_status), exit, &err);

exit:
    if (tc1_status) free(tc1_status);
    return err;
}

static int HttpSetSocketStatus(httpd_request_t *req)
{
    OSStatus err = kNoErr;

    int buf_size = 512;
    char *buf = malloc(buf_size);

    err = httpd_get_data(req, buf, buf_size);
    require_noerr(err, exit);

    SetSocketStatus(buf);

    send_http("OK", 2, exit, &err);

exit:
    if (buf) free(buf);
    return err;
}

static int HttpGetPowerInfo(httpd_request_t *req)
{
    OSStatus err = kNoErr;
    char buf[16];
    err = httpd_get_data(req, buf, 16);
    require_noerr(err, exit);

    int idx = 0;
    sscanf(buf, "%d", &idx);

    //计算系统运行时间
    mico_time_t past_ms = 0;
    mico_time_get_time(&past_ms);
    int past = past_ms / 1000;
    int h = past / 3600;
    int m = past / 60 % 60;
    int s = past % 60;
    sprintf(up_time, "%d:%02d:%02d", h, m, s);

    char* powers = GetPowerRecord(idx);
    sprintf(power_info_json, POWER_INFO_JSON, power_record.idx, PW_NUM, p_count, powers, up_time);
    send_http(power_info_json, strlen(power_info_json), exit, &err);
exit:
    return err;
}

static int HttpGetWifiConfig(httpd_request_t *req)
{
    OSStatus err = kNoErr;
    const unsigned char* status = GetSocketStatus();
    send_http(status, strlen((char*)status), exit, &err);
exit:
    return err;
}

static int HttpSetWifiConfig(httpd_request_t *req)
{
    OSStatus err = kNoErr;

    int buf_size = 97;
    char *buf = malloc(buf_size);
    char *wifi_ssid = malloc(32);
    char *wifi_key = malloc(64);

    err = httpd_get_data(req, buf, buf_size);
    require_noerr(err, exit);

    sscanf(buf, "%s %s", wifi_ssid, wifi_key);

    WifiConnect(wifi_ssid, wifi_key);

    send_http("OK", 2, exit, &err);

exit:
    if (buf) free(buf);
    if (wifi_ssid) free(wifi_ssid);
    if (wifi_key) free(wifi_key);
    return err;
}

static int HttpGetWifiScan(httpd_request_t *req)
{
    OSStatus err = kNoErr;
    if (scaned)
    {
        scaned = false;
        send_http(wifi_ret, strlen(wifi_ret), exit, &err);
        free(wifi_ret);
    }
    else
    {
        send_http("NO", 2, exit, &err);
    }

exit:
    return err;
}

static int HttpSetWifiScan(httpd_request_t *req)
{
    micoWlanStartScanAdv();
    OSStatus err = kNoErr;
    send_http("OK", 2, exit, &err);
exit:
    return err;
}

static int HttpGetLog(httpd_request_t *req)
{
    OSStatus err = kNoErr;
    char* logs = GetLogRecord(0);
    send_http(logs, strlen(logs), exit, &err);

exit:
    return err;
}

static int HttpGetTasks(httpd_request_t *req)
{
    /*
    pTimedTask pt = (pTimedTask)malloc(sizeof(struct TimedTask));
    pt->prs_time = time(NULL) + 5;
    pt->socket_idx = 5;
    pt->on = 0;
    AddTask(pt);
    */

    OSStatus err = kNoErr;
    char* tasks_str = GetTaskStr();
    send_http(tasks_str, strlen(tasks_str), exit, &err);

exit:
    if (tasks_str) free(tasks_str);
    return err;
}

static int HttpAddTask(httpd_request_t *req)
{
    OSStatus err = kNoErr;

    //157736962320 4 0
    char buf[16] = { 0 };
    err = httpd_get_data(req, buf, 16);
    require_noerr(err, exit);

    pTimedTask task = (pTimedTask)malloc(sizeof(struct TimedTask));
    sscanf(buf, "%ld %d %d", &task->prs_time, &task->socket_idx, &task->on);

    char* mess = AddTask(task) ? "OK" : "NO";

    send_http(mess, strlen(mess), exit, &err);
exit:
    return err;
}

static int HttpDelTask(httpd_request_t *req)
{
    //TODO 从url获取参数
    char buf[16] = "1234567"; //假设已经获取到了.

    int time;
    sscanf(buf, "%d", &time);

    char* mess = DelTask(time) ? "OK" : "NO";

    OSStatus err = kNoErr;
    send_http(mess, strlen(mess), exit, &err);
exit:
    return err;
}

struct httpd_wsgi_call g_app_handlers[] = {
    {"/", HTTPD_HDR_DEFORT, 0, HttpGetIndexPage, NULL, NULL, NULL},
    {"/socket", HTTPD_HDR_DEFORT, 0, NULL, HttpSetSocketStatus, NULL, NULL},
    {"/status", HTTPD_HDR_DEFORT, 0, HttpGetTc1Status, NULL, NULL, NULL},
    {"/power", HTTPD_HDR_DEFORT, 0, NULL, HttpGetPowerInfo, NULL, NULL},
    {"/wifi/config", HTTPD_HDR_DEFORT, 0, HttpGetWifiConfig, HttpSetWifiConfig, NULL, NULL},
    {"/wifi/scan", HTTPD_HDR_DEFORT, 0, HttpGetWifiScan, HttpSetWifiScan, NULL, NULL },
    {"/log", HTTPD_HDR_DEFORT, 0, HttpGetLog, NULL, NULL, NULL},
    {"/task", HTTPD_HDR_DEFORT, 0, HttpGetTasks, HttpAddTask, NULL, HttpDelTask },
};

static int g_app_handlers_no = sizeof(g_app_handlers)/sizeof(struct httpd_wsgi_call);

static void AppHttpRegisterHandlers()
{
    int rc;
    rc = httpd_register_wsgi_handlers(g_app_handlers, g_app_handlers_no);
    if (rc) {
        app_httpd_log("failed to register test web handler");
    }
}

static int _AppHttpdStart()
{
    OSStatus err = kNoErr;
    app_httpd_log("initializing web-services");

    /*Initialize HTTPD*/
    if(is_http_init == false) {
        err = httpd_init();
        require_noerr_action(err, exit, app_httpd_log("failed to initialize httpd"));
        is_http_init = true;
    }

    /*Start http thread*/
    err = httpd_start();
    if(err != kNoErr) {
        app_httpd_log("failed to start httpd thread");
        httpd_shutdown();
    }
exit:
    return err;
}

int AppHttpdStart(void)
{
    OSStatus err = kNoErr;

    err = _AppHttpdStart();
    require_noerr(err, exit);

    if (is_handlers_registered == false) {
        AppHttpRegisterHandlers();
        is_handlers_registered = true;
    }

exit:
    return err;
}

int AppHttpdStop()
{
    OSStatus err = kNoErr;

    /* HTTPD and services */
    app_httpd_log("stopping down httpd");
    err = httpd_stop();
    require_noerr_action(err, exit, app_httpd_log("failed to halt httpd"));

exit:
    return err;
}
