#pragma once
#include <time.h>

#define POS_WEEKDAY 0xF
#define POS_ON 0xF0
#define POS_SOCKET_IDX 0xF00

struct TimedTask;
typedef struct TimedTask* pTimedTask;
struct TimedTask
{
    unsigned char id;
    bool on_use;     //正在使用
    time_t prs_time; //被执行的格林尼治时间戳
    unsigned char socket_idx;  //要控制的插孔:0-5
    unsigned char on;          //开或者关:0-1
    unsigned char weekday;     //星期重复:0-8 0代表不重复 8代表每日重复
    pTimedTask next; //下一个任务(按时间排序)
};

pTimedTask NewTask();
bool AddTask(pTimedTask task);
bool DelTask(short task_id);
bool DelFirstTask();
void ProcessTask();
// bool GetTaskStr();
bool FlushTask();
