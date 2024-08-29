#include "main.h"

#include "user_gpio.h"
#include "user_wifi.h"
#include "time_server/user_rtc.h"
#include "user_power.h"
#include "mqtt_server/user_mqtt_client.h"
#include "http_server/app_httpd.h"
#include "timed_task/timed_task.h"

char rtc_init = 0; //sntp校时成功标志位
uint32_t total_time = 0;
char str_mac[16] = { 0 };

system_config_t* sys_config;
user_config_t* user_config;

mico_gpio_t Relay[Relay_NUM] = { Relay_0, Relay_1, Relay_2, Relay_3, Relay_4, Relay_5 };

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback(void * const user_config_data, uint32_t size)
{
    UNUSED_PARAMETER(size);

    mico_system_context_get()->micoSystemConfig.name[0] = 1; //在下次重启时使用默认名称
    mico_system_context_get()->micoSystemConfig.name[1] = 0;

    user_config_t* userConfigDefault = user_config_data;
    userConfigDefault->user[0] = 0;
    userConfigDefault->mqtt_ip[0] = 0;
    userConfigDefault->mqtt_port = 0;
    userConfigDefault->mqtt_user[0] = 0;
    userConfigDefault->mqtt_password[0] = 0;
    userConfigDefault->task_top = NULL;
    // userConfigDefault->task_count = 0;
    userConfigDefault->version = USER_CONFIG_VERSION;

    int i;
    for (i = 0; i < SOCKET_NUM; i++)
    {
        userConfigDefault->socket_status[i] = 1;
    }
    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        userConfigDefault->timed_tasks[i].id = i;
        userConfigDefault->timed_tasks[i].on_use = false;
        userConfigDefault->timed_tasks[i].next = NULL;
    }
    //mico_system_context_update(sys_config);
}

int application_start(void)
{
    int i;
    tc1_log("start version[%s]", VERSION);

    //char main_num=0;
    OSStatus err = kNoErr;

    // Create mico system context and read application's config data from flash
    sys_config = mico_system_context_init(sizeof(user_config_t));
    user_config = ((system_context_t*)sys_config)->user_config_data;
    require_action(user_config, exit, err = kNoMemoryErr);

    err = mico_system_init(sys_config);
    require_noerr(err, exit);

    uint8_t mac[8];
    mico_wlan_get_mac_address(mac);
    sprintf(str_mac, "%02X%02X%02X%02X%02X%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    tc1_log("str_mac[%s]", str_mac);

    bool open_ap = false;
    MicoGpioInitialize((mico_gpio_t)Button, INPUT_PULL_UP);
    if (!MicoGpioInputGet(Button))
    {   //开机时按钮状态
        tc1_log("press ap_init");
        ApInit(false);
        open_ap = true;
    }

    MicoGpioInitialize((mico_gpio_t) Led, OUTPUT_PUSH_PULL);
    for (i = 0; i < Relay_NUM; i++)
    {
        MicoGpioInitialize(Relay[i], OUTPUT_PUSH_PULL);
        UserRelaySet(i, user_config->socket_status[i]);
    }
    MicoSysLed(0);

    if (user_config->version != USER_CONFIG_VERSION)
    {
        tc1_log("WARNGIN: user params restored!");
        err = mico_system_context_restore(sys_config);
        require_noerr(err, exit);
    }

    if (sys_config->micoSystemConfig.name[0] == 1)
    {
        sprintf(sys_config->micoSystemConfig.name, ZTC1_NAME, str_mac+8);
    }

    tc1_log("user:%s",user_config->user);
    tc1_log("device name:%s",sys_config->micoSystemConfig.name);
    tc1_log("mqtt_ip:%s",user_config->mqtt_ip);
    tc1_log("mqtt_port:%d",user_config->mqtt_port);
    tc1_log("mqtt_user:%s",user_config->mqtt_user);
    //tc1_log("mqtt_password:%s",user_config->mqtt_password);
    tc1_log("version:%d",user_config->version);

    WifiInit();
    if (!open_ap)
    {
        if (sys_config->micoSystemConfig.reserved != NOTIFY_STATION_UP)
        {
            ApInit(true);
        }
        else
        {
            WifiConnect(sys_config->micoSystemConfig.ssid,
                sys_config->micoSystemConfig.user_key);
        }
    }
    KeyInit();
    err = UserMqttInit();
    require_noerr(err, exit);
    err = UserRtcInit();
    require_noerr(err, exit);
    PowerInit();
    AppHttpdStart(); // start http server thread

    while (1)
    {
        UserMqttHassPower();

        time_t now = time(NULL);
        if (user_config->task_top && now >= user_config->task_top->prs_time)
        {
            ProcessTask();
        }
        mico_thread_msleep(1000);
    }

exit:
    tc1_log("application_start ERROR!");
    return 0;
}

