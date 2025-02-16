/*
 * data.c
 *
 *  Created on: 2015??7??9??
 *      Author: jk
 */


#include "data.h"
#include "math.h"

typedef struct queue
{
    int front;
    int rear;
    GPS gps[MAX_GPS_COUNT];
}QUEUE;

static QUEUE gps_queue = {0, 0};
static u32 BatteryVoltage[MAX_VLOTAGE_NUM] = {0};
static LOCAL_GPS last_gps_info;
static LOCAL_GPS* last_gps = &last_gps_info;

static char isItineraryStart = ITINERARY_END;
int VibrationTime = 0;

/*
 * to judge whether the queue is full
 */
eat_bool gps_isQueueFull(void)
{
    return gps_queue.front == (gps_queue.rear + 1) % MAX_GPS_COUNT;
}

/*
 * to judge whether the queue is empty
 */
eat_bool gps_isQueueEmpty(void)
{
    return gps_queue.front == gps_queue.rear;
}

/*
 * put a GPS in the queue
 */
eat_bool gps_enqueue(GPS* gps)
{
    if (gps_isQueueFull())
    {
        return EAT_FALSE;
    }

    /*memcpy is not useful, dont know why*/
    //memcpy(gps_queue.gps[gps_queue.rear], gps, sizeof(GPS));

    gps_queue.gps[gps_queue.rear].timestamp= gps->timestamp;
    gps_queue.gps[gps_queue.rear].longitude = gps->longitude;
    gps_queue.gps[gps_queue.rear].latitude = gps->latitude;
    gps_queue.gps[gps_queue.rear].speed = gps->speed;
    gps_queue.gps[gps_queue.rear].course = gps->course;

    gps_queue.rear = (gps_queue.rear + 1) % MAX_GPS_COUNT;

    return EAT_TRUE;
}


/*
 * get a GPS in the queue
 */
eat_bool gps_dequeue(GPS* gps)
{
    if (!gps)
    {
        return EAT_FALSE;
    }

    if (gps_isQueueEmpty())
    {
        return EAT_FALSE;
    }

    /*memcpy is not useful, dont know why*/
    //memcpy(gps, gps_queue.gps[gps_queue.front], sizeof(GPS));

    gps->timestamp = gps_queue.gps[gps_queue.front].timestamp;
    gps->longitude = gps_queue.gps[gps_queue.front].longitude;
    gps->latitude = gps_queue.gps[gps_queue.front].latitude;
    gps->speed = gps_queue.gps[gps_queue.front].speed;
    gps->course = gps_queue.gps[gps_queue.front].course;

    gps_queue.front = (gps_queue.front + 1) % MAX_GPS_COUNT;

    return EAT_TRUE;
}

/*
 * get the queue's actual size
 */
int gps_size(void)
{
    return (gps_queue.rear - gps_queue.front + MAX_GPS_COUNT) % MAX_GPS_COUNT;
}

/*
*get the last gps info
*/
LOCAL_GPS* gps_get_last(void)
{
    return last_gps;
}

/*
*save the last gps info
*/
int gps_save_last(LOCAL_GPS* gps)
{
    last_gps_info.isGps = gps->isGps;
    if(gps->isGps)
    {
        last_gps_info.gps.timestamp = gps->gps.timestamp;
        last_gps_info.gps.latitude = gps->gps.latitude;
        last_gps_info.gps.longitude = gps->gps.longitude;
        last_gps_info.gps.speed = gps->gps.speed;
        last_gps_info.gps.course = gps->gps.course;
    }
    else
    {
        last_gps_info.cellInfo = gps->cellInfo;
    }

    return EAT_TRUE;
}


/*
*set the battery's voltage
*/
void battery_store_voltage(u32 voltage)
{
    static int count = 0;

    if(count >= MAX_VLOTAGE_NUM)
    {
        count = 0;
    }

    BatteryVoltage[count++] = voltage;
}

/*
* get the battery's voltage
*/
unsigned char battery_get_percent(void)
{
#define ADvalue_2_Realvalue(x) x*103/3/1000.f //unit mV, 3K & 100k divider
#define Voltage2Percent(x) (unsigned char)exp((x-37.873)/2.7927)
    u32 voltage = 0;
    int count;
    unsigned char percent;

    for(count = 0;count < MAX_VLOTAGE_NUM;count++)
    {
        voltage += BatteryVoltage[count];
    }

    voltage /= MAX_VLOTAGE_NUM;

    if(ADvalue_2_Realvalue(voltage) > 55)
    {
        voltage = voltage*48/60;    //normalizing to 48V
    }
    else if(ADvalue_2_Realvalue(voltage) > 40)
    {
        voltage = voltage;
    }
    else if(ADvalue_2_Realvalue(voltage) > 28)
    {
        voltage = voltage*48/36;    //normalizing to 48V
    }

    percent = Voltage2Percent(ADvalue_2_Realvalue(voltage));

    return percent>100?100:percent;
}

unsigned char battery_get_miles(void)
{
    return 0;
}

/*
*vibration time ,for autolock & initerary
*/
int getVibrationTime(void)
{
    return VibrationTime;
}
/*
*Add once a sec
*/
int VibrationTimeAdd(void)
{
    return VibrationTime++;
}
/*
*if move or set defendoff ,reset the time
*/
int ResetVibrationTime(void)
{
    return VibrationTime = 0;
}

char get_itinerary_state(void)
{
    return isItineraryStart;
}

void set_itinerary_state(char state)
{
    isItineraryStart = state;
}


