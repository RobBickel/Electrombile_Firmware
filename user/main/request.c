/*
 * request.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#include <string.h>

#include <eat_interface.h>

#include "request.h"
#include "setting.h"
#include "msg.h"
#include "log.h"
#include "socket.h"
#include "version.h"
#include "data.h"

int cmd_Login(void)
{
    MSG_LOGIN_REQ* msg = alloc_msg(CMD_LOGIN, sizeof(MSG_LOGIN_REQ));
    u8 imei[IMEI_LENGTH] = {0};

    if (!msg)
    {
        LOG_ERROR("alloc login message failed!");
        return -1;
    }

    msg->version = htonl(VERSION_NUM);

    eat_get_imei(imei, IMEI_LENGTH);

    memcpy(msg->IMEI, imei, IMEI_LENGTH);

    LOG_DEBUG("send login message.");
    socket_sendData(msg, sizeof(MSG_LOGIN_REQ));

    return 0;
}

void cmd_Heartbeat(void)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(short);
    MSG_PING_REQ* msg = alloc_msg(CMD_PING, msgLen);
    msg->status = htons(EAT_TRUE);   //TODO: to define the status bits

    socket_sendData(msg, msgLen);
}

int cmd_Itinerary(void)
{
    MSG_ITINERARY_REQ* itinerary_msg;
    MSG_ITINERARY_REQ itinerary;
    int rc = -1;

    rc = itinerary_get(&itinerary);

    if(rc == -9)//no file ,no itinerary storage
    {
        return 0;
    }

    if(rc < 0)
    {
        LOG_ERROR("read itinerary file error!");
    }

    itinerary_msg = alloc_msg(CMD_ITINERARY, sizeof(MSG_ITINERARY_REQ));

    itinerary_msg->starttime = itinerary.starttime;
    itinerary_msg->mileage= itinerary.mileage;
    itinerary_msg->endtime= itinerary.endtime;

    LOG_DEBUG("send itinerary msg,start:%d end:%d itinerary:%d",ntohl(itinerary_msg->starttime),ntohl(itinerary_msg->endtime),ntohl(itinerary_msg->mileage));

    socket_sendData((MSG_ITINERARY_REQ*)itinerary_msg, sizeof(MSG_ITINERARY_REQ));

    return 0;
}

int cmd_SMS(const void* msg)
{
    return 0;
}


void cmd_Wild(const void* m, int len)
{
    u8 msgLen = sizeof(MSG_HEADER) + len;
    MSG_HEADER* msg = alloc_msg(CMD_WILD, msgLen);

    memcpy(msg + 1, m, len);

    socket_sendData(msg, msgLen);
}

int cmd_Seek(unsigned int value)
{
    MSG_433* seek_msg = alloc_msg(CMD_433, sizeof(MSG_433));
    if (!seek_msg)
    {
        LOG_ERROR("alloc seek message failed!");
        return -1;
    }

    LOG_DEBUG("send seek value message:%d", value);
    seek_msg->intensity = htonl(value);
    socket_sendData(seek_msg, sizeof(MSG_433));

    return 0;
}

int cmd_GPS(GPS* gps)
{
    MSG_GPS* msg = alloc_msg(CMD_GPS, sizeof(MSG_GPS));
    if (!msg)
    {
        LOG_ERROR("alloc GPS message failed!");
        return -1;
    }

    memcpy(&msg->gps, gps, sizeof(GPS));
    msg->gps.timestamp = htonl(gps->timestamp);
    msg->gps.course = htons(gps->course);

    LOG_DEBUG("send GPS message.");

    socket_sendData(msg, sizeof(MSG_GPS));

    return 0;
}

int cmd_GPSPack(void)
{
    //TODO: send all the gps data in the queue
    u8 msgLen = sizeof(MSG_HEADER) + gps_size()*sizeof(GPS);
    MSG_GPS_PACK* msg;
    int count = 0;

    if(gps_isQueueEmpty())//if queue is empty , do not send gps msg
    {
        return 0;
    }

    LOG_DEBUG("msgLen %d, sizeof(GPS) %d", msgLen, sizeof(GPS));

    msg = alloc_msg(CMD_GPS_PACK, msgLen);
    if (!msg)
    {
        LOG_ERROR("alloc GPSPack message failed!");
        return -1;
    }

    for(count = 0;count < MAX_GPS_COUNT;count++)
    {
        if(gps_isQueueEmpty())
            break;

        gps_dequeue(&(msg->gps[count]));
        msg->gps[count].timestamp = htonl(msg->gps[count].timestamp);
        msg->gps[count].course = htons(msg->gps[count].course);
    }

    socket_sendData(msg, msgLen);

    return 0;

}

