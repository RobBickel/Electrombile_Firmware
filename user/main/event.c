/*
 * event.c
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */
#include <string.h>

#include <eat_interface.h>
#include <eat_uart.h>

#include "timer.h"
#include "thread_msg.h"
#include "log.h"
#include "uart.h"
#include "socket.h"
#include "setting.h"
#include "diagnosis.h"
#include "msg.h"
#include "client.h"
#include "modem.h"
#include "fsm.h"
#include "request.h"
#include "seek.h"
#include "data.h"
#include "adc.h"
#include "initerary.h"

typedef int (*EVENT_FUNC)(const EatEvent_st* event);
typedef struct
{
	EatEvent_enum event;
	EVENT_FUNC pfn;
}EVENT_PROC;

typedef int (*THREAD_MSG_FUNC)(const MSG_THREAD* msg);
typedef struct
{
    char cmd;
    THREAD_MSG_FUNC pfn;
}THREAD_MSG_PROC;




#define DESC_DEF(x) case x:\
                            return #x

static char* getEventDescription(EatEvent_enum event)
{
    switch (event)
    {
        #ifdef LOG_DEBUG_FLAG
                DESC_DEF(EAT_EVENT_TIMER);
                DESC_DEF(EAT_EVENT_KEY);
                DESC_DEF(EAT_EVENT_INT);
                DESC_DEF(EAT_EVENT_MDM_READY_RD);
                DESC_DEF(EAT_EVENT_MDM_READY_WR);
                DESC_DEF(EAT_EVENT_MDM_RI);
                DESC_DEF(EAT_EVENT_UART_READY_RD);
                DESC_DEF(EAT_EVENT_UART_READY_WR);
                DESC_DEF(EAT_EVENT_ADC);
                DESC_DEF(EAT_EVENT_UART_SEND_COMPLETE);
                DESC_DEF(EAT_EVENT_USER_MSG);
                DESC_DEF(EAT_EVENT_IME_KEY);
        #endif
        default:
        {
            static char soc_event[10] = {0};
            sprintf(soc_event, "%d", event);
            return soc_event;
        }
    }
}


static int event_mod_ready_rd(const EatEvent_st* event)
{
	u8 buf[256] = {0};
	u16 len = 0;

	len = eat_modem_read(buf, 256);
	if (!len)
	{
	    LOG_ERROR("modem received nothing.");
	    return -1;
	}
    LOG_DEBUG("modem recv: %s", buf);

    if (modem_IsCallReady(buf))
    {
        diag_check();

        fsm_run(EVT_CALL_READY);
    }


	return 0;
}

static int event_timer(const EatEvent_st* event)
{
    switch (event->data.timer.timer_id)
    {
        case TIMER_LOOP:
            LOG_DEBUG("TIMER_LOOP expire.");
            fsm_run(EVT_LOOP);
            eat_timer_start(event->data.timer.timer_id, setting.main_loop_timer_period);
            break;

        case TIMER_SEEKAUTOOFF:
            LOG_DEBUG("TIMER_SEEKAUTOOFF expire!");
            setSeekMode(EAT_FALSE);
            break;

        case TIMER_GPS_SEND:
            cmd_GPSPack();
            eat_timer_start(event->data.timer.timer_id, setting.gps_send_period);
            break;

        default:
            LOG_ERROR ("timer(%d) not processed!", event->data.timer.timer_id);
            break;
    }
    return 0;
}

static int event_adc(const EatEvent_st* event)
{
    unsigned int value = event->data.adc.v;

    LOG_DEBUG("ad value=%d", value);

    if (event->data.adc.pin == ADC_433)
    {
        seek_proc(value);
    }
    else
    {
        LOG_INFO("not processed adc pin:%d", event->data.adc.pin);
    }

    return 0;
}

static void sendGPS2Server(LOCAL_GPS* gps)
{

    if (gps->isGps)
    {
        cmd_GPS(&gps->gps);
    }
#if 0
    else
    {
        size_t msgLen = sizeof(MSG_HEADER) + sizeof(CGI) + sizeof(CELL) * gps->cellInfo.cellNo;
        MSG_HEADER* msg = alloc_msg(CMD_CELL, msgLen);
        CGI* cgi = (CGI*)(msg + 1);
        CELL* cell = (CELL*)(cgi + 1);
        int i = 0;
        if (!msg)
        {
            LOG_ERROR("alloc CELL message failed!");
            return;
        }

        cgi->mcc = htons(gps->cellInfo.mcc);
        cgi->mnc = htons(gps->cellInfo.mnc);
        cgi->cellNo = gps->cellInfo.cellNo;
        for (i = 0; i < gps->cellInfo.cellNo; i++)
        {
            cell[i].lac = htons(gps->cellInfo.cell[i].lac);
            cell[i].cellid = htons(gps->cellInfo.cell[i].cellid);
            cell[i].rxl= htons(gps->cellInfo.cell[i].rxl);
        }

        LOG_DEBUG("send CELL message.");
        socket_sendData(msg, msgLen);

        data.isCellGet = EAT_FALSE;
    }
#endif
}

static int threadCmd_GPS(const MSG_THREAD* msg)
{
    LOCAL_GPS* gps = (LOCAL_GPS*) msg->data;

     if (msg->length < sizeof(LOCAL_GPS)  || !gps)
     {
         LOG_ERROR("msg from THREAD_GPS error!");
         return -1;
     }


     if (gps->isGps)
     {
         gps_enqueue(&gps->gps);
     }

     if (gps_isQueueFull())
     {
         cmd_GPSPack();
     }

    return 0;
}

static int threadCmd_AutolockState(const MSG_THREAD* msg)
{
    AUTOLOCK_INFO* msg_state = (AUTOLOCK_INFO*) msg->data;
    MSG_AUTODEFEND_STATE_REQ* autolock_msg;

    if (msg->length < sizeof(AUTOLOCK_INFO) || !msg_state)
    {
         LOG_ERROR("msg from THREAD_VIBRATION error!");
         return -1;
    }
    autolock_msg = alloc_msg(CMD_DEFEND_NOTIFY, sizeof(MSG_AUTODEFEND_STATE_REQ));
    autolock_msg->state = msg_state->state;

    LOG_DEBUG("send auto lock state change message: %d", msg_state->state);
    socket_sendData(autolock_msg, sizeof(MSG_AUTODEFEND_STATE_REQ));

    return 0;
}

/*
*fun: receive msg from GPS_Thread and send itinerary msg to server
*/
static int threadCmd_Itinerary(const MSG_THREAD* msg)
{
    GPS_ITINERARY_INFO* msg_data = (GPS_ITINERARY_INFO*) msg->data;
    MSG_ITINERARY_REQ* itinerary_msg;
    MSG_ITINERARY_REQ* itinerary;
    int rc = -1;
    msg_data->itinerary = 666;
    if(msg_data->itinerary <= 0)
    {
        LOG_DEBUG("miles is 0,do not send msg!");
        return 0;
    }

    if (msg->length < sizeof(GPS_ITINERARY_INFO) || !msg_data)
    {
         LOG_ERROR("msg from THREAD_GPS error!");
         return -1;
    }
    itinerary_msg = alloc_msg(CMD_ITINERARY, sizeof(MSG_ITINERARY_REQ));
    itinerary_msg->starttime = htonl(msg_data->starttime);
    itinerary_msg->endtime = htonl(msg_data->endtime);
    itinerary_msg->mileage = htonl(msg_data->itinerary);

    itinerary = alloc_msg(CMD_ITINERARY, sizeof(MSG_ITINERARY_REQ));
    itinerary->starttime = htonl(msg_data->starttime);
    itinerary->endtime = htonl(msg_data->endtime);
    itinerary->mileage = htonl(msg_data->itinerary);

    LOG_DEBUG("send itinerary msg,start:%d end:%d itinerary:%d",msg_data->starttime,msg_data->endtime,msg_data->itinerary);
    rc = socket_sendData((MSG_ITINERARY_REQ*)itinerary_msg, sizeof(MSG_ITINERARY_REQ));
    if(rc < 0)
    {
        rc = itinerary_store(itinerary);
        itinerary->mileage = 555;
    }
    else
    {
        rc = itinerary_get(itinerary);
        LOG_DEBUG("%d,%d,%d",ntohl(itinerary->starttime),ntohl(itinerary->mileage),ntohl(itinerary->endtime));
    }
    freeMsg(itinerary);

    return 0;
}


static int threadCmd_SMS(const MSG_THREAD* msg)
{
    LOG_DEBUG("receive thread command CMD_SMS.");

    return 0;
}

static int threadCmd_Vibrate(const MSG_THREAD* msg)
{
    unsigned char* alarm_type = (unsigned char*)msg->data;
    MSG_ALARM_REQ* socket_msg = 0;

    if (msg->length != sizeof(*alarm_type))
    {
        LOG_ERROR("msg length error: msgLen(%d)!", msg->length);
        return -1;
    }
    LOG_DEBUG("receive thread command CMD_VIBRATE: alarmType(%d).", *alarm_type);

    socket_msg = alloc_msg(CMD_ALARM, sizeof(MSG_ALARM_REQ));
    if (!socket_msg)
    {
        LOG_ERROR("alloc message failed!");
        return -1;
    }

    LOG_DEBUG("send alarm vibrate message.");
    socket_msg->alarmType = *alarm_type;
    socket_sendData(socket_msg, sizeof(MSG_ALARM_REQ));

    return 0;
}


static int threadCmd_Location(const MSG_THREAD* msg)
{
    LOCAL_GPS* gps = (LOCAL_GPS*) msg->data;

    if (msg->length < sizeof(LOCAL_GPS)  || !gps)
    {
        LOG_ERROR("msg from THREAD_GPS error!");
        return -1;
    }

    if (gps->isGps)             //update the local GPS data
    {
        MSG_GPSLOCATION_RSP* msg = alloc_msg(CMD_LOCATE, sizeof(MSG_GPSLOCATION_RSP));
        if (!msg)
        {
            LOG_ERROR("alloc message failed!");
            return -1;
        }
        msg->isGps= gps->isGps;
        memcpy(&msg->gps, &gps->gps, sizeof(GPS));

        msg->gps.timestamp = htonl(gps->gps.timestamp);
        msg->gps.course = htons(gps->gps.course);

        LOG_DEBUG("send GPS_LOCATION message.");
        socket_sendData(msg, sizeof(MSG_GPSLOCATION_RSP));
    }
    else                //update local cell info
    {
        size_t msgLen = sizeof(MSG_CELLLOCATION_HEADER) + sizeof(CGI) + sizeof(CELL) * gps->cellInfo.cellNo;
        MSG_CELLLOCATION_HEADER* msg = alloc_msg(CMD_LOCATE, msgLen);
        CGI* cgi = (CGI*)(msg + 1);
        CELL* cell = (CELL*)(cgi + 1);
        int i = 0;

        if (!msg)
        {
            LOG_ERROR("alloc message failed!");
            return -1;
        }
        msg->isGps = gps->isGps;

        cgi->mcc = htons(gps->cellInfo.mcc);
        cgi->mnc = htons(gps->cellInfo.mnc);
        cgi->cellNo = gps->cellInfo.cellNo;
        for (i = 0; i < gps->cellInfo.cellNo; i++)
        {
            cell[i].lac = htons(gps->cellInfo.cell[i].lac);
            cell[i].cellid = htons(gps->cellInfo.cell[i].cellid);
            cell[i].rxl= htons(gps->cellInfo.cell[i].rxl);
        }

        LOG_DEBUG("send CELL_LOCATION message.");
        socket_sendData(msg, msgLen);
    }

    return 0;
}


static THREAD_MSG_PROC msgProcs[] =
{
        {CMD_THREAD_GPS, threadCmd_GPS},
        {CMD_THREAD_SMS, threadCmd_SMS},
        {CMD_THREAD_VIBRATE, threadCmd_Vibrate},
        {CMD_THREAD_LOCATION, threadCmd_Location},
        {CMD_THREAD_AUTOLOCK, threadCmd_AutolockState},
        {CMD_THREAD_ITINERARY, threadCmd_Itinerary},
};

static int event_threadMsg(const EatEvent_st* event)
{
    MSG_THREAD* msg = (MSG_THREAD*) event->data.user_msg.data_p;
    u8 msgLen = event->data.user_msg.len;
    size_t i = 0;
    int rc = 0;

    if (msg->length + sizeof(MSG_THREAD) != msgLen)
    {
        LOG_ERROR("Message length error");
        freeMsg(msg);

        return -1;
    }

    for (i = 0; i < sizeof(msgProcs) / sizeof(msgProcs[0]); i++)
    {
        if (msgProcs[i].cmd == msg->cmd)
        {
            THREAD_MSG_FUNC pfn = msgProcs[i].pfn;
            if (pfn)
            {
                rc = pfn(msg);
                break;
            }
            else
            {
                LOG_ERROR("thread message %d not processed!", msg->cmd);
                rc = -1;
                break;
            }
        }
    }

    freeMsg(msg);

    return rc;
}




static EVENT_PROC eventProcs[] =
{
    {EAT_EVENT_TIMER,               event_timer},
    {EAT_EVENT_MDM_READY_RD,        event_mod_ready_rd},
    {EAT_EVENT_MDM_READY_WR,        EAT_NULL},
    {EAT_EVENT_UART_READY_RD,       event_uart_ready_rd},
    {EAT_EVENT_UART_READY_WR,       event_uart_ready_wr},
    {EAT_EVENT_UART_SEND_COMPLETE,  EAT_NULL},
    {EAT_EVENT_USER_MSG,            event_threadMsg},
    {EAT_EVENT_ADC,                 event_adc},
};


int event_proc(EatEvent_st* event)
{
    int i = 0;

    LOG_DEBUG("event: %s happened", getEventDescription(event->event));

    for (i = 0; i < sizeof(eventProcs) / sizeof(eventProcs[0]); i++)
    {
        if (eventProcs[i].event == event->event)
        {
            EVENT_FUNC pfn = eventProcs[i].pfn;
            if (pfn)
            {
                return pfn(event);
            }
            else
            {
                LOG_ERROR("event(%s) not processed!", getEventDescription(event->event));
                return -1;
            }
        }
    }

    LOG_ERROR("event(%s) has no handler!", getEventDescription(event->event));

    return -1;
}
