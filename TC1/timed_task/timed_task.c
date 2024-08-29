#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "main.h"
#include "user_gpio.h"
#include "timed_task/timed_task.h"
#include "http_server/web_log.h"

int day_sec = 86400;

pTimedTask NewTask()
{
    for (int i = 0; i < MAX_TASK_NUM; i++)
    {
        pTimedTask task = &user_config->timed_tasks[i];
        if (!task->on_use)
        {
            task->on_use = true;
            return task;
        }
    }
    return NULL;
}

bool AddTaskSingle(pTimedTask new_task)
{
    // user_config->task_count++;
    if (user_config->task_top == NULL)
    {
        new_task->next = NULL;
        user_config->task_top = new_task;
        return true;
    }

    if (new_task->prs_time <= user_config->task_top->prs_time)
    {
        // 插入到最前面
        new_task->next = user_config->task_top;
        user_config->task_top = new_task;
        return true;
    }

    pTimedTask p_tsk = user_config->task_top;
    while (p_tsk != NULL)
    {
        if (p_tsk->next == NULL || (new_task->prs_time > p_tsk->prs_time && new_task->prs_time <= p_tsk->next->prs_time))
        {
            new_task->next = p_tsk->next;
            p_tsk->next = new_task;
            return true;
        }
        p_tsk = p_tsk->next;
    }
    // user_config->task_count--;
    return false;
}

bool AddTaskWeek(pTimedTask task)
{
    time_t now = time(NULL);
    int today_weekday = (now / day_sec + 3) % 7 + 1; // 1970-01-01 星期五
    int next_day = task->weekday - today_weekday;
    bool next_day_is_today = next_day == 0 && task->prs_time % day_sec > now % day_sec;
    next_day = next_day > 0 || next_day_is_today ? next_day : next_day + 7;
    task->prs_time = (now - now % day_sec) + (next_day * day_sec) + task->prs_time % day_sec;

    return AddTaskSingle(task);
}

bool AddTask(pTimedTask task)
{
    if (task->weekday == 0 || task->weekday == 8)
        return AddTaskSingle(task);
    return AddTaskWeek(task);
}

bool DelFirstTask()
{
    if (user_config->task_top)
    {
        pTimedTask tmp = user_config->task_top;
        user_config->task_top = user_config->task_top->next;
        // user_config->task_count--;
        if (tmp->weekday == 0)
        {
            tmp->on_use = false;
        }
        else if (tmp->weekday == 8) // 8代表每日任务
        {
            tmp->prs_time += day_sec;
            AddTask(tmp);
        }
        else
        {
            tmp->prs_time += 7 * day_sec;
            AddTask(tmp);
        }
        return true;
    }
    return false;
}

bool DelTask(short task_id)
{
    if (user_config->task_top->id == task_id)
    {
        user_config->task_top = user_config->task_top->next;
        // user_config->task_count--;
        return true;
    }
    pTimedTask pre_tsk = user_config->task_top;
    pTimedTask p_tsk = user_config->task_top->next;
    while (p_tsk != NULL)
    {
        if (p_tsk->id == task_id)
        {
            pre_tsk->next = p_tsk->next;

            p_tsk->next = NULL;
            p_tsk->on_use = false;
            // user_config->task_count--;
            return true;
        }
        pre_tsk = p_tsk;
        p_tsk = p_tsk->next;
    }
    return false;
}

void ProcessTask()
{
    task_log("process task time[%ld] socket_idx[%d] on[%d]",
             user_config->task_top->prs_time, user_config->task_top->socket_idx, user_config->task_top->on);
    UserRelaySet(user_config->task_top->socket_idx, user_config->task_top->on);
    DelFirstTask();
}
/*
bool GetTaskStr()
{
    OSStatus err = kNoErr;
    // char* str = (char*)malloc(sizeof(char)*(user_config->task_count*89+2));
    pTimedTask p_tsk = user_config->task_top;
    send_http_chunked_header(exit, &err);
    send_http_chunked_data('[', 1, exit, &err);
    unsigned char i = 0;
    while (p_tsk)
    {
        if (i++ != 0) {
            send_http_chunked_data(',', 1, exit, &err);
        }
        char t_str[75];
        sprintf(t_str, "{'id':%d,'timestamp':%ld,'socket_idx':%d,'on':%d,'weekday':%d}",
            p_tsk->id,p_tsk->prs_time, p_tsk->socket_idx, p_tsk->on, p_tsk->weekday);
        send_http_chunked_data(t_str, strlen(t_str), exit, &err);
    }
    send_http_chunked_data(']', 1, exit, &err);
    send_http_chunked_data(NULL, 0, exit, &err);

    // char* tmp_str = str;
    // tmp_str[0] = '[';
    // tmp_str[2] = 0;
    // tmp_str++;

    // while (tmp_tsk)
    // {
    //     char buffer[26];
    //     struct tm* tm_info;
    //     time_t prs_time = tmp_tsk->prs_time + 28800;
    //     tm_info = localtime(&prs_time);
    //     strftime(buffer, 26, "%m-%d %H:%M", tm_info);

    //     sprintf(tmp_str, "{'id':%d,'timestamp':%ld,'prs_time':'%s','socket_idx':%d,'on':%d,'weekday':%d},",
    //         tmp_tsk->id,tmp_tsk->prs_time, buffer, tmp_tsk->socket_idx+1, tmp_tsk->on, tmp_tsk->weekday);
    //     tmp_str += strlen(tmp_str);
    //     tmp_tsk = tmp_tsk->next;
    // }
    // if (strlen(tmp_str) > 0) tmp_str[strlen(tmp_str)-1] = 0;
    // if (user_config->task_count > 0) --tmp_str;
    // *tmp_str = ']';
    // return str;

exit:
    return err;
}
*/
bool FlushTask()
{
    int i;

    user_config->task_top = NULL;
    // user_config->task_count = 0;
    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        user_config->timed_tasks[i].id = i;
        user_config->timed_tasks[i].on_use = false;
        user_config->timed_tasks[i].next = NULL;
    }
    return true;
}