/*
 * msg.c
 *
 *  Created on: 2015??7??8??
 *      Author: jk
 */
#include <eat_interface.h>
#include <eat_mem.h>

#include "msg.h"

static char seq = 0;

void* alloc_msg(char cmd, size_t length)
{
    MSG_HEADER* msg = eat_mem_alloc(length);

    if (msg)
    {
        msg->signature = htons(START_FLAG);
        msg->cmd = cmd;
        msg->seq = seq++;
        msg->length = htons(length - MSG_HEADER_LEN);
    }

    return msg;
}

void* alloc_rspMsg(const MSG_HEADER* pMsg)
{
    MSG_HEADER* msg = NULL;
    int i = 0;
    size_t msgLen = 0;
    typedef struct
    {
        char cmd;
        size_t size;
    }MSG_LENGTH;


    MSG_LENGTH rsp_msg_length_map[] =
    {
            //{CMD_DEFEND,    sizeof(MSG_DEFEND_RSP)},
            {CMD_SEEK,      sizeof(MSG_SEEK_RSP)},

            {CMD_SET_AUTOSWITCH,    sizeof(MSG_AUTODEFEND_SWITCH_SET_RSP)},
            {CMD_GET_AUTOSWITCH,    sizeof(MSG_AUTODEFEND_SWITCH_GET_RSP)},

            {CMD_SET_PERIOD,    sizeof(MSG_AUTODEFEND_PERIOD_SET_RSP)},
            {CMD_GET_PERIOD,    sizeof(MSG_AUTODEFEND_PERIOD_GET_RSP)},

            {CMD_SET_TIMER,   sizeof(MSG_SET_TIMER_RSP)},
            {CMD_DEFEND_ON,   sizeof(MSG_DEFEND_ON_RSP)},
            {CMD_DEFEND_OFF,   sizeof(MSG_DEFEND_OFF_RSP)},
            {CMD_DEFEND_GET,   sizeof(MSG_DEFEND_GET_RSP)},

            {CMD_UPGRADE_START, sizeof(MSG_UPGRADE_START_RSP)},
            {CMD_UPGRADE_DATA,  sizeof(MSG_UPGRADE_DATA_RSP)},
            {CMD_UPGRADE_END,   sizeof(MSG_UPGRADE_END_RSP)},
            {CMD_REBOOT,        sizeof(MSG_HEADER)},
            {CMD_BATTERY,   sizeof(MSG_BATTERY_RSP)},
    };

    for (i = 0; i < sizeof(rsp_msg_length_map) / sizeof(rsp_msg_length_map[0]); i++)
    {
        if (pMsg->cmd == rsp_msg_length_map[i].cmd)
        {
            msgLen = rsp_msg_length_map[i].size;
        }
    }

    if (msgLen == 0)
    {
        return NULL;
    }

    msg = eat_mem_alloc(msgLen);
    if (!msg)
    {
        return msg;
    }
    msg->signature = htons(START_FLAG);
    msg->cmd = pMsg->cmd;
    msg->length = htons(msgLen - MSG_HEADER_LEN);
    msg->seq = pMsg->seq;

    return msg;
}


void free_msg(void* msg)
{
    eat_mem_free(msg);
}

