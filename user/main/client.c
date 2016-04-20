/*
 * client.c
 *
 *  Created on: 2015/7/9
 *      Author: jk
 */

#include <stdio.h>
#include <string.h>


#include <eat_interface.h>

#include "client.h"
#include "socket.h"
#include  "msg.h"
#include "log.h"
#include "uart.h"
#include "setting.h"
#include "thread.h"
#include "thread_msg.h"
#include "timer.h"
#include "seek.h"

#include "response.h"





typedef int (*MSG_PROC)(const void* msg);       //TODO: add the message length parameter
typedef struct
{
    char cmd;
    MSG_PROC pfn;
}MC_MSG_PROC;


static MC_MSG_PROC msgProcs[] =
{
    {CMD_LOGIN, cmd_Login_rsp},
    {CMD_PING,  cmd_Ping_rsp},
    {CMD_ALARM, cmd_Alarm_rsp},
    {CMD_SMS,   cmd_Sms_rsp},
    {CMD_SEEK,  cmd_Seek_rsp},
	{CMD_LOCATE, cmd_Location_rsp},
	{CMD_SET_AUTOSWITCH, cmd_AutodefendSwitchSet_rsp},
    {CMD_GET_AUTOSWITCH, cmd_AutodefendSwitchGet_rsp},
    {CMD_SET_PERIOD, cmd_AutodefendPeriodSet_rsp},
    {CMD_GET_PERIOD, cmd_AutodefendPeriodGet_rsp},
    {CMD_SET_SERVER, cmd_Server_rsp},
    {CMD_SET_TIMER, cmd_Timer_rsp},
    {CMD_BATTERY, cmd_Battery_rsp},
    {CMD_DEFEND_OFF, cmd_DefendOff_rsp},
    {CMD_DEFEND_ON, cmd_DefendOn_rsp},
    {CMD_DEFEND_GET, cmd_DefendGet_rsp},
    {CMD_UPGRADE_START, cmd_UpgradeStart_rsp},
    {CMD_UPGRADE_DATA, cmd_UpgradeData_rsp},
    {CMD_UPGRADE_END, cmd_UpgradeEnd_rsp},
    {CMD_REBOOT, cmd_Reboot_rsp},
    {CMD_DEVICE_INFO_GET, cmd_DeviceInfo_rsp},
    {CMD_ITINERARY, cmd_Itinerary_rsp},
};

int client_handleOnePkt(const void* m, int msgLen)
{
    MSG_HEADER* msg = (MSG_HEADER*)m;
    size_t i = 0;

    for (i = 0; i < sizeof(msgProcs) / sizeof(msgProcs[0]); i++)
    {
        if (msgProcs[i].cmd == msg->cmd)
        {
            MSG_PROC pfn = msgProcs[i].pfn;
            if (pfn)
            {
                return pfn(msg);
            }
            else
            {
                LOG_ERROR("Message %d not processed!", msg->cmd);
                return -1;
            }
        }
    }

    LOG_ERROR("unknown message %d!", msg->cmd);
    return -1;
}

int client_proc(const void *m, int msgLen)
{
    const MSG_HEADER *msg = (const MSG_HEADER *)m;
    size_t leftLen = 0;

    LOG_HEX(m, msgLen);

    if(msgLen < MSG_HEADER_LEN)
    {
        LOG_ERROR("message length not enough: %zu(at least(%zu)", msgLen, sizeof(MSG_HEADER));
        return -1;
    }

    leftLen = msgLen;
    while(leftLen >= ntohs(msg->length) + MSG_HEADER_LEN)
    {
        if (ntohs(msg->signature) != START_FLAG)
        {
            LOG_ERROR("receive message header signature error:%#x", (unsigned)ntohs(msg->signature));
            return -1;
        }

        client_handleOnePkt(msg, ntohs(msg->length) + MSG_HEADER_LEN);
        leftLen = leftLen - MSG_HEADER_LEN - ntohs(msg->length);
        msg = (const MSG_HEADER *)((const char *)m + msgLen - leftLen);
//        LOG_HEX((const char *)msg, leftLen);
    }
    return 0;
}


