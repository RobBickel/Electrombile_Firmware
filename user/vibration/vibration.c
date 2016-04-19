/*
 * vibration.c
 *
 *  Created on: 2015??7??1??
 *      Author: jk
 */

#include <stdlib.h>
#include <eat_interface.h>
#include <eat_periphery.h>

#include "vibration.h"
#include "thread.h"
#include "log.h"
#include "timer.h"
#include "thread_msg.h"
#include "setting.h"
#include "mma8652.h"
#include "led.h"
#include "data.h"

#define MAX_MOVE_DATA_LEN   500
#define MOVE_TIMER_PERIOD    10

#define MOVE_THRESHOLD 5

static eat_bool avoid_freq_flag = EAT_FALSE;
static int AlarmCount = 0;

eat_bool isMoved = EAT_FALSE;

void DigitalIntegrate(float * sour, float * dest,int len,float cycle)
{
	int i;
	if (len==0)
		return ;
	dest[0]=0;
	for (i=1;i<len;i++)
	{
		dest[i]=dest[i-1]+(sour[i-1]+sour[i])*cycle/2;
	}
}


static eat_bool vivration_AutolockStateSend(eat_bool state)
{
    eat_bool ret;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(AUTOLOCK_INFO);
    MSG_THREAD* msg = allocMsg(msgLen);
    AUTOLOCK_INFO* msg_state = 0;

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }

    msg->cmd = CMD_THREAD_AUTOLOCK;
    msg->length = sizeof(AUTOLOCK_INFO);

    msg_state = (AUTOLOCK_INFO*)msg->data;
    msg_state->state = state;

    LOG_DEBUG("send autolock state msg to main thread!");
    ret = sendMsg(THREAD_MAIN, msg, msgLen);

    return ret;
}

static eat_bool vibration_sendAlarm(void)
{
    u8 msgLen = sizeof(MSG_THREAD) + 1;
    MSG_THREAD* msg = NULL;
    unsigned char* alarmType = NULL;

    if(AlarmCount++ <= 3)
    {
        msg = allocMsg(msgLen);
        alarmType = (unsigned char*)msg->data;

        msg->cmd = CMD_THREAD_VIBRATE;
        msg->length = 1;
        *alarmType = ALARM_VIBRATE;

        LOG_DEBUG("vibration alarm:cmd(%d),length(%d),data(%d)", msg->cmd, msg->length, *(unsigned char*)msg->data);
        avoid_freq_flag = EAT_TRUE;
        return sendMsg(THREAD_MAIN, msg, msgLen);
    }
}

/*
*fun:send itinerary state to gps thread
*note:ITINERARY_START express itinerary start, ITINERARY_END express itinerary end
*/
static eat_bool vivration_SendItinerarayState(char state)
{
    eat_bool ret;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(VIBRATION_ITINERARY_INFO);
    MSG_THREAD* msg = allocMsg(msgLen);
    VIBRATION_ITINERARY_INFO* msg_state = 0;

    if (!msg)
    {
        LOG_ERROR("alloc itinerary msg failed!");
        return EAT_FALSE;
    }

    msg->cmd = CMD_THREAD_ITINERARY;
    msg->length = sizeof(VIBRATION_ITINERARY_INFO);

    msg_state = (VIBRATION_ITINERARY_INFO*)msg->data;
    msg_state->state = state;

    LOG_DEBUG("send itinerary state msg to GPS_thread:%d",state);
    set_itinerary_state(state);
    ret = sendMsg(THREAD_GPS, msg, msgLen);


    return ret;
}

static void move_alarm_timer_handler()
{
    unsigned char readbuf[3];
    int i;
    float tmp[3]={0};
    short temp;
    static float x_data[MAX_MOVE_DATA_LEN], y_data[MAX_MOVE_DATA_LEN], z_data[MAX_MOVE_DATA_LEN];
    float temp_data[MAX_MOVE_DATA_LEN];
    static int timerCount = 0;
    bool ret = mma8652_i2c_register_read(MMA8652_REG_OUT_X_MSB, readbuf, 3);
    temp = readbuf[0]<<8;
    x_data[timerCount] = temp/256;
     temp = readbuf[1]<<8;
    y_data[timerCount] = temp/256;
     temp = readbuf[2]<<8;
    z_data[timerCount] = temp/256;
    timerCount++;

    if(timerCount<MAX_MOVE_DATA_LEN)
    {
        eat_timer_start(TIMER_MOVE_ALARM, MOVE_TIMER_PERIOD);
    }
    else
    {
        timerCount = 0;
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            tmp[0] += (x_data[i]/MAX_MOVE_DATA_LEN);
            tmp[1] += (y_data[i]/MAX_MOVE_DATA_LEN);
            tmp[2] += (z_data[i]/MAX_MOVE_DATA_LEN);
        }
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            x_data[i] = x_data[i] - tmp[0];
            y_data[i] = y_data[i] - tmp[1];
            z_data[i] = z_data[i] - tmp[2];
        }
        DigitalIntegrate(x_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, x_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(x_data[0]<abs(x_data[i]))
            {
                x_data[0] = abs(x_data[i]);
            }
            if(x_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_sendAlarm();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, x_data[i]);
                }

                if(ITINERARY_END == get_itinerary_state())
                {
                    vivration_SendItinerarayState(ITINERARY_START);
                }

               return;
            }

        }
        LOG_DEBUG("MAX_X  = %f", x_data[0]);
        DigitalIntegrate(y_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);

        DigitalIntegrate(temp_data, y_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(y_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_sendAlarm();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, y_data[i]);
                }

                if(ITINERARY_END == get_itinerary_state())
                {
                    vivration_SendItinerarayState(ITINERARY_START);
                }

                return;
            }
            if(y_data[0]<abs(y_data[i]))
            {
                y_data[0] = abs(y_data[i]);
            }

        }
        LOG_DEBUG("MAX_Y  = %f", y_data[0]);
        DigitalIntegrate(z_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, z_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(z_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_sendAlarm();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, z_data[i]);
                }

                if(ITINERARY_END == get_itinerary_state())
                {
                    vivration_SendItinerarayState(ITINERARY_START);
                }

                return;
            }
            if(z_data[0]<abs(z_data[i]))
            {
                z_data[0] = abs(z_data[i]);
            }
        }
        LOG_DEBUG("MAX_z  = %f", z_data[0]);
    }


}

static void avoid_fre_send(eat_bool state)
{
    static u16 avoid_freq_count;

    if(state == EAT_TRUE)
    {
        if(++avoid_freq_count == 10)
        {
            avoid_freq_count = 0;
            avoid_freq_flag = EAT_FALSE;
        }
    }
    else
    {
        avoid_freq_count = 0;
    }
}


static void vibration_timer_handler(void)
{
    static eat_bool isFirstTime = EAT_TRUE;

    static int timerCount = 0;
    uint8_t transient_src = 0;

    avoid_fre_send(EAT_TRUE);

    mma8652_i2c_register_read(MMA8652_REG_TRANSIENT_SRC, &transient_src, sizeof(transient_src));
    if(transient_src & MMA8652_TRANSIENT_SRC_EA)
    {
        /* At the first time, the value of MMA8X5X_TRANSIENT_SRC is strangely 0x60.
         * Do not send alarm at the first time.
         */
        if(isFirstTime)
        {
            isFirstTime = EAT_FALSE;

            isMoved = EAT_FALSE;
        }
        else
        {
            isMoved = EAT_TRUE;
        }
    }
    else
    {
        isMoved = EAT_FALSE;
    }

    //always to judge if need to alarm , just judge the defend state before send alarm
    if(isMoved && avoid_freq_flag == EAT_FALSE)
    {
        avoid_fre_send(EAT_FALSE);
        eat_timer_start(TIMER_MOVE_ALARM, MOVE_TIMER_PERIOD);
        //vibration_sendAlarm();  //bec use displacement judgement , there do not alarm
    }

    if(isMoved)
    {
        ResetVibrationTime();
        LOG_DEBUG("shake!");
    }
    else
    {
        VibrationTimeAdd();

        if(getVibrationTime() * setting.vibration_timer_period >= (get_autodefend_period() * 60000))
        {
            if(get_autodefend_state())
            {
                if(EAT_FALSE == vibration_fixed())
                {
                    vivration_AutolockStateSend(EAT_TRUE);    //TODO:send autolock_msg to main thread
                    set_vibration_state(EAT_TRUE);
                }
            }

            if(ITINERARY_START == get_itinerary_state())
            {
                vivration_SendItinerarayState(ITINERARY_END);
            }

            if(AlarmCount > 0)
            {
                AlarmCount = 0;
            }

        }
    }

    return;
}


void app_vibration_thread(void *data)
{
	EatEvent_st event;
	bool ret;

	LOG_INFO("vibration thread start.");

	ret = mma8652_init();
	if (!ret)
	{
        LED_off();
        LOG_ERROR("mma8652 init failed");
	}
	else
	{
	    mma8652_config();
	}

	eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);

	while(EAT_TRUE)
	{
        eat_get_event_for_user(THREAD_VIBRATION, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER:
                switch (event.data.timer.timer_id)
                {
                    case TIMER_VIBRATION:
                        vibration_timer_handler();
                        eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);
                        break;
                    case TIMER_MOVE_ALARM:
                        move_alarm_timer_handler();
                        break;

                    default:
                        LOG_ERROR("timer(%d) expire!", event.data.timer.timer_id);
                        break;
                }
                break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;
        }
    }
}


