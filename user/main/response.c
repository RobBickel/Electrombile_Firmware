/*
 * response.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */
#include <string.h>
#include <stdint.h>


#include <eat_interface.h>

#include "response.h"
#include "msg.h"
#include "log.h"
#include "thread_msg.h"
#include "thread.h"
#include "socket.h"
#include "fsm.h"
#include "version.h"
#include "upgrade.h"
#include "adler32.h"
#include "data.h"
#include "fs.h"
#include "setting.h"
#include "battery.h"
#include "seek.h"
#include "itinerary.h"

int cmd_Login_rsp(const void* msg)
{
    LOG_DEBUG("get login respond.");

    cmd_Itinerary();//if logined , check if there is a itinerary exist, if Y send it

    fsm_run(EVT_LOGINED);

    return 0;
}

int cmd_Ping_rsp(const void* msg)
{
    LOG_DEBUG("get ping respond.");

    return 0;
}

int cmd_Alarm_rsp(const void* msg)
{
    MSG_ALARM_REQ* req = (MSG_ALARM_REQ*)msg;
    //MSG_ALARM_RSP* rsp = NULL;

    switch(req->alarmType)
    {
        case ALARM_VIBRATE:
            LOG_DEBUG("get alarm(ALARM_VIBRATE) respond.");
            break;

        case ALARM_FENCE_OUT:
            LOG_DEBUG("get alarm(ALARM_FENCE_OUT) respond.");
            break;

        case ALARM_FENCE_IN:
            LOG_DEBUG("get alarm(ALARM_FENCE_IN) respond.");
            break;

        default:
            break;
    }

    return 0;
}

int cmd_Sms_rsp(const void* msg)
{
    return 0;
}

int cmd_Itinerary_rsp(const void* msg)
{
    MSG_HEADER* req = (MSG_HEADER*)msg;
    MSG_HEADER* rsp = NULL;

    LOG_DEBUG("ititerary send ok.");
    itinerary_delete();

    return 0;
}

int cmd_Reboot_rsp(const void* msg)
{
    MSG_HEADER* req = (MSG_HEADER*)msg;
    MSG_HEADER* rsp = NULL;

    LOG_DEBUG("reboot.");
    rsp = alloc_rspMsg(req);
    if (!rsp)
    {
        LOG_ERROR("alloc reboot rsp message failed!");
        return -1;
    }

    socket_sendData(rsp, sizeof(MSG_DEFEND_REQ));

    eat_reset_module();
    return 0;
}


int cmd_Seek_rsp(const void* msg)
{
    MSG_SEEK_REQ* req = (MSG_SEEK_REQ*)msg;
    MSG_SEEK_RSP* rsp = NULL;

    if (req->operator == SEEK_ON)
    {
        setSeekMode(EAT_TRUE);

        LOG_DEBUG("set seek on.");
    }
    else if(req->operator == SEEK_OFF)
    {
        setSeekMode(EAT_FALSE);
        LOG_DEBUG("set seek off.");
    }

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc seek rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_SEEK_RSP));

    return 0;
}

int cmd_Location_rsp(const void* msgLocation)
{
    u8 msgLen = sizeof(MSG_THREAD);
    MSG_THREAD* msg = allocMsg(msgLen);

    msg->cmd = CMD_THREAD_LOCATION;
    msg->length = 0;

    LOG_DEBUG("send CMD_THREAD_LOCATION to THREAD_GPS.");
    sendMsg(THREAD_GPS, msg, msgLen);

    return 0;
}

int cmd_AutodefendSwitchSet_rsp(const void* msg)
{
    MSG_AUTODEFEND_SWITCH_SET_REQ* req = (MSG_AUTODEFEND_SWITCH_SET_REQ*)msg;
    MSG_AUTODEFEND_SWITCH_SET_RSP* rsp = NULL;

    if(req->onOff == AUTO_DEFEND_ON)
    {
        set_autodefend_state(EAT_TRUE);
        LOG_DEBUG("set autodefend swtich on.");
    }
    else if(req->onOff == AUTO_DEFEND_OFF)
    {
        set_autodefend_state(EAT_FALSE);
        LOG_DEBUG("set autodefend swtich off.");
    }

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_swtich_set rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_SWITCH_SET_RSP));

    return 0;
}

int cmd_AutodefendSwitchGet_rsp(const void* msg)
{
    MSG_AUTODEFEND_SWITCH_GET_REQ* req = (MSG_AUTODEFEND_SWITCH_GET_REQ*)msg;
    MSG_AUTODEFEND_SWITCH_GET_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_swtich_get rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = get_autodefend_state() ? AUTO_DEFEND_ON : AUTO_DEFEND_OFF;

    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_SWITCH_GET_RSP));

    return 0;
}

int cmd_AutodefendPeriodSet_rsp(const void* msg)
{
    MSG_AUTODEFEND_PERIOD_SET_REQ* req = (MSG_AUTODEFEND_PERIOD_SET_REQ*)msg;
    MSG_AUTODEFEND_PERIOD_SET_RSP* rsp = NULL;

    LOG_DEBUG("set autodefend period as %dmins.", req->period);
    set_autodefend_period(req->period);

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_period_set rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_PERIOD_SET_RSP));

    return 0;
}

int cmd_AutodefendPeriodGet_rsp(const void* msg)
{
    MSG_AUTODEFEND_PERIOD_GET_REQ* req = (MSG_AUTODEFEND_PERIOD_GET_REQ*)msg;
    MSG_AUTODEFEND_PERIOD_GET_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_period_get rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->period = get_autodefend_period();
    LOG_DEBUG("send autodefend period: %d minutes",rsp->period);
    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_PERIOD_GET_RSP));

    return 0;
}

int cmd_Battery_rsp(const void* msg)
{
    MSG_BATTERY_RSP* req = (MSG_BATTERY_RSP*)msg;
    MSG_BATTERY_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc baterry rsp message failed!");
        return -1;
    }
    rsp->percent = battery_get_percent();
    rsp->miles = battery_get_miles();

    LOG_DEBUG("send battery msg to server:%d",rsp->percent);
    socket_sendData(rsp, sizeof(MSG_BATTERY_RSP));

    return 0;
}


int cmd_DefendOn_rsp(const void* msg)
{
    MSG_DEFEND_REQ* req = (MSG_DEFEND_REQ*)msg;
    MSG_DEFEND_RSP* rsp = NULL;
    int result = MSG_SUCCESS;

    LOG_DEBUG("set defend switch on.");

    set_vibration_state(EAT_TRUE);
    if(EAT_TRUE != vibration_fixed())
    {
        result = -1;
    }

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = result;

    socket_sendData(rsp, sizeof(MSG_DEFEND_RSP));

    return 0;
}

int cmd_DefendOff_rsp(const void* msg)
{
    MSG_DEFEND_REQ* req = (MSG_DEFEND_REQ*)msg;
    MSG_DEFEND_RSP* rsp = NULL;
    int result = MSG_SUCCESS;

    LOG_DEBUG("set defend switch off.");

    ResetVibrationTime();
    set_vibration_state(EAT_FALSE);
    if(EAT_FALSE != vibration_fixed())
    {
        result = -1;
    }

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = result;

    socket_sendData(rsp, sizeof(MSG_DEFEND_REQ));

    return 0;
}
int cmd_DefendGet_rsp(const void* msg)
{
    MSG_DEFEND_REQ* req = (MSG_DEFEND_REQ*)msg;
    MSG_DEFEND_RSP* rsp = NULL;
    unsigned char result = MSG_SUCCESS;

    result = vibration_fixed() ? DEFEND_ON : DEFEND_OFF;
    LOG_DEBUG("get defend switch state(%d).", result);

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = result;

    socket_sendData(rsp, sizeof(MSG_DEFEND_RSP));

    return 0;
}

int cmd_Timer_rsp(const void* msg)
{
    MSG_SET_TIMER_REQ* req = (MSG_SET_TIMER_REQ*)msg;
    MSG_SET_TIMER_RSP* rsp = NULL;
    if(0 >= ntohl(req->timer))
    {
        ;//rsp at the end
    }
    else if(10 >= ntohl(req->timer))
    {
        setting_save();

    }
    else if(21600 <= ntohl(req->timer))
    {
        setting_save();

    }
    else if((10 < ntohl(req->timer))&&(21600 > ntohl(req->timer)))
    {

        setting_save();

    }
    rsp = alloc_rspMsg(&req->header);

    //TODO: fix the gps upload time
    rsp->result = htonl(30);
    socket_sendData(rsp,sizeof(MSG_SET_TIMER_RSP));

    return 0;
}

int cmd_Server_rsp(const void* msg)
{
    MSG_SET_SERVER* msg_server = (MSG_SET_SERVER*)msg;
    u32 ip[4] = {0};
    int count;
    signed char domain[MAX_DOMAIN_NAME_LEN] = {0};

    count = sscanf(msg_server->server,"%u.%u.%u.%u",&ip[0],&ip[1],&ip[2],&ip[3]);
    if(4 == count)
    {
        setting.addr_type = ADDR_TYPE_IP;
        setting.ipaddr[0] = (u8)ip[0];
        setting.ipaddr[1] = (u8)ip[1];
        setting.ipaddr[2] = (u8)ip[2];
        setting.ipaddr[3] = (u8)ip[3];
        setting.port = (u16)ntohl(msg_server->port);

        setting_save();
        LOG_DEBUG("server proc %s:%d successful!",msg_server->server,msg_server->port);

        eat_reset_module();
    }
    else
    {
        count = sscanf(msg_server->server, "%[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.]", domain);
        if(1 == count)
        {
            setting.addr_type = ADDR_TYPE_DOMAIN;
            strcpy(setting.domain, msg_server->server);
            setting.port = (u16)ntohl(msg_server->port);

            setting_save();
            LOG_DEBUG("server proc %s:%d successful!",msg_server->server,msg_server->port);

            eat_reset_module();
        }
        else
        {
            LOG_DEBUG("server proc %s:%d error!",msg_server->server,msg_server->port);
        }
    }

    return 0;
}

int cmd_UpgradeStart_rsp(const void* msg)
{
    MSG_UPGRADE_START* req = (MSG_UPGRADE_START*)msg;
    MSG_UPGRADE_START_RSP* rsp = NULL;
    int rc = 0;
    SINT64 freeDiskSize = 0;

    LOG_DEBUG("new app size : %d;new version : now version = %d:%d",ntohl(req->size),ntohl(req->version),VERSION_NUM);

    if (ntohl(req->version) <= VERSION_NUM)    //No need to upgrade, normally not happened
    {
        rc = MSG_VERSION_NOT_SUPPORTED;
        LOG_DEBUG("No need to upgrade%d:%d",ntohl(req->version),VERSION_NUM);
    }
    else
    {
        upgrade_DeleteOldApp();
        freeDiskSize = fs_getDiskFreeSize();
        if (ntohl(req->size) >= freeDiskSize)      //TODO: equal is not enough, maybe should reserve some disk space
        {
            LOG_ERROR("Free disk is not enough , can not start app_upgrade!");
            rc = MSG_DISK_NO_SPACE;
        }
        else
        {
            LOG_DEBUG("Free disk is enough , create file to start app_upgrade!");
            //创建升级包文件
            rc = upgrade_createFile();
        }
    }

    //回应是否可以升级
    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc rsp msg failed:cmd =%d", req->header.cmd);
        return -1;
    }
    rsp->code = rc;
    socket_sendData(rsp,sizeof(MSG_UPGRADE_START_RSP));

    return 0;
}

int cmd_UpgradeData_rsp(const void* msg)
{
    MSG_UPGRADE_DATA* req = (MSG_UPGRADE_DATA*)msg;
    MSG_UPGRADE_DATA_RSP* rsp = NULL;
    int rc = 0;
    int expectLength = 0;
    short data_length ;
    int data_offset = ntohl(req->offset);

    data_length = htons(req->header.length) - sizeof(req->offset);

    LOG_DEBUG("data length:%d , data offset:%d",data_length,data_offset);

    rc = upgrade_appendFile(data_offset, req->data, data_length);

    if (rc >= 0)
    {
        expectLength = data_offset + data_length;
        LOG_DEBUG("success this time, and expect length is:%d",expectLength);
    }
    else
    {
        expectLength = data_offset;
        LOG_DEBUG("error this time, and expect length is:%d",expectLength);
    }

    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc rsp msg failed:cmd=%d", req->header.cmd);
        return -1;
    }

    rsp->offset = htonl(expectLength);
    socket_sendData(rsp,sizeof(MSG_UPGRADE_DATA_RSP));

    return rc;
}

int cmd_UpgradeEnd_rsp(const void* msg)
{
    MSG_UPGRADE_END* req = (MSG_UPGRADE_END*)msg;
    MSG_UPGRADE_END_RSP* rsp = NULL;

    int rc =  upgrade_CheckAppfile(ntohl(req->size),ntohl(req->checksum));

    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc rsp msg failed:cmd=%d", req->header.cmd);
        return -1;
    }
    rsp->code = rc;
    socket_sendData(rsp,sizeof(MSG_UPGRADE_END_RSP));

    //启动升级
    if(0 <= rc)
    {
        rc = upgrade_do();
    }

    return rc;
}

int cmd_DeviceInfo_rsp(const void* msg)
{
    MSG_DEVICE_INFO_GET_REQ* req = (MSG_DEVICE_INFO_GET_REQ*)msg;
    MSG_DEVICE_INFO_GET_RSP* rsp = NULL;
    int msgLen;
    LOCAL_GPS* local_gps = gps_get_last();

    if(local_gps->isGps)    //becaus of auto malloc ram,cant use alloc_rspMsg
    {
        msgLen = sizeof(MSG_DEVICE_INFO_GET_RSP);
    }
    else
    {
        msgLen = sizeof(MSG_DEVICE_INFO_GET_RSP)-sizeof(GPS) + 4*sizeof(short);
    }

    rsp = eat_mem_alloc(msgLen);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->header.signature = htons(START_FLAG);
    rsp->header.cmd = req->cmd;
    rsp->header.length = htons(msgLen - MSG_HEADER_LEN);
    rsp->header.seq = req->seq;

    rsp->isGps = local_gps->isGps;
    rsp->autolock = setting.isAutodefendFixed;
    rsp->autoperiod = setting.autodefendPeriod;
    rsp->defend = setting.isVibrateFixed;
    rsp->percent = battery_get_percent();
    rsp->miles = battery_get_miles();

    if(rsp->isGps)
    {
        rsp->gps.timestamp = htonl(local_gps->gps.timestamp);
        rsp->gps.latitude = local_gps->gps.latitude;
        rsp->gps.longitude = local_gps->gps.longitude;
        rsp->gps.speed = local_gps->gps.speed;
        rsp->gps.course = htons(local_gps->gps.course);
    }
    else
    {
        rsp->mcc = htons(local_gps->cellInfo.mcc);
        rsp->mnc = htons(local_gps->cellInfo.mnc);
        rsp->lac = htons(local_gps->cellInfo.cell[0].lac);
        rsp->cid = htons(local_gps->cellInfo.cell[0].cellid);
    }

    socket_sendData(rsp,msgLen);
    return 0;
}


