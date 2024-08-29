#ifndef __MAIN_H_
#define __MAIN_H_

#include "mico.h"
#include "micokit_ext.h"
#include "timed_task/timed_task.h"

#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__); web_log("APP", M, ##__VA_ARGS__);
#define key_log(M, ...) custom_log("KEY", M, ##__VA_ARGS__); web_log("KEY", M, ##__VA_ARGS__);
#define ota_log(M, ...) custom_log("OTA", M, ##__VA_ARGS__); web_log("OTA", M, ##__VA_ARGS__);
#define rtc_log(M, ...) custom_log("RTC", M, ##__VA_ARGS__); web_log("RTC", M, ##__VA_ARGS__);
#define tc1_log(M, ...) custom_log("TC1", M, ##__VA_ARGS__); web_log("TC1", M, ##__VA_ARGS__);
#define task_log(M, ...) custom_log("TASK", M, ##__VA_ARGS__); web_log("TASK", M, ##__VA_ARGS__);
#define http_log(M, ...) custom_log("HTTP", M, ##__VA_ARGS__); web_log("HTTP", M, ##__VA_ARGS__);
#define mqtt_log(M, ...) custom_log("MQTT", M, ##__VA_ARGS__); web_log("MQTT", M, ##__VA_ARGS__);
#define wifi_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__); web_log("WIFI", M, ##__VA_ARGS__);
#define power_log(M, ...) custom_log("POWER", M, ##__VA_ARGS__); web_log("POWER", M, ##__VA_ARGS__);

#define VERSION "v2.1.67"

#define TYPE 1
#define TYPE_NAME "zTC1"

#define ZTC1_NAME "zTC1-%s"

#define USER_CONFIG_VERSION 8
#define SETTING_MQTT_STRING_LENGTH_MAX 32 //必须4字节对齐。

#define SOCKET_NAME_LENGTH   32
#define SOCKET_NUM           6  //插座数量

#define Led    MICO_GPIO_5
#define Button MICO_GPIO_23
#define POWER  MICO_GPIO_15

#define Relay_ON  1
#define Relay_OFF 0

#define Relay_0   MICO_GPIO_6
#define Relay_1   MICO_GPIO_8
#define Relay_2   MICO_GPIO_10
#define Relay_3   MICO_GPIO_7
#define Relay_4   MICO_GPIO_9
#define Relay_5   MICO_GPIO_18
#define Relay_NUM SOCKET_NUM

#define MAX_TASK_NUM 128

//用户保存参数结构体
typedef struct
{
    char version;
    char mqtt_ip[SETTING_MQTT_STRING_LENGTH_MAX];
    int mqtt_port;
    char mqtt_user[SETTING_MQTT_STRING_LENGTH_MAX];
    char mqtt_password[SETTING_MQTT_STRING_LENGTH_MAX];
    char socket_status[SOCKET_NUM]; //记录当前开关
    char user[maxNameLen];
    WiFiEvent last_wifi_status;
    char ap_name[32];
    char ap_key[32];
    // char task_count;
    pTimedTask task_top;
    struct TimedTask timed_tasks[MAX_TASK_NUM];
} user_config_t;

extern char rtc_init;
extern uint32_t total_time;
extern char str_mac[16];
extern system_config_t* sys_config;
extern user_config_t* user_config;
extern mico_gpio_t Relay[Relay_NUM];

#endif
