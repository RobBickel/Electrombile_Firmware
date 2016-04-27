/*
 * protocol.h
 *
 *  Created on: 2015/6/29
 *      Author: jk
 *
 *  Copyright (c) 2015 Wuhan Xiaoan Technology Co., Ltd. All rights reserved.
 *
 *  Change log:
 *      2.15    去掉CMD_LOGIN中的CCID字段
 *              增加CMD_SIM_INFO命令字
 *
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

enum
{
    PROTOCOL_VESION_251 = 215,
};

#define PROTOCOL_VERSION    PROTOCOL_VESION_251

#define START_FLAG (0xAA55)

#define IMEI_LENGTH 15
#define IMSI_LENGTH 15

#define MAX_CCID_LENGTH 20
#define MAX_CELL_NUM 7

#define TEL_NO_LENGTH 11


enum
{

    CMD_WILD            =  0,
    CMD_LOGIN           =  1,
    CMD_PING            =  2,
    CMD_GPS             =  3,
    CMD_CELL            =  4,
    CMD_ALARM           =  5,
    CMD_SMS             =  6,
    CMD_433             =  7,
    CMD_DEFEND          =  8,
    CMD_SEEK            =  9,
    CMD_LOCATE          = 10,
    CMD_SET_SERVER      = 11,
    CMD_SET_TIMER       = 12,
    CMD_SET_AUTOSWITCH  = 13,
    CMD_GET_AUTOSWITCH  = 14,
    CMD_SET_PERIOD      = 15,
    CMD_GET_PERIOD      = 16,
    CMD_ITINERARY       = 17,
    CMD_BATTERY         = 18,
    CMD_DEFEND_ON       = 19,
    CMD_DEFEND_OFF      = 20,
    CMD_DEFEND_GET      = 21,
    CMD_DEFEND_NOTIFY   = 22,
    CMD_UPGRADE_START   = 23,
    CMD_UPGRADE_DATA    = 24,
    CMD_UPGRADE_END     = 25,
    CMD_SIM_INFO        = 26,
    CMD_REBOOT          = 27,
    CMD_DEVICE_INFO_GET = 28,
    CMD_GPS_PACK        = 29,
};

enum
{
    MSG_SUCCESS = 0,
    MSG_VERSION_NOT_SUPPORTED = -1,
    MSG_DISK_NO_SPACE = -2,
    MSG_UPGRADE_CHECKSUM_FAILED = -3,
};

#pragma pack(push, 1)
/*
 * Message header definition
 */
typedef struct
{
    short signature;
    char cmd;
    char seq;
    short length;
}__attribute__((__packed__)) MSG_HEADER;

#define MSG_HEADER_LEN sizeof(MSG_HEADER)

/*
 * Login message structure
 */
typedef struct
{
    MSG_HEADER header;
    int version;
    char deciveType;
    char IMEI[IMEI_LENGTH];
}__attribute__((__packed__)) MSG_LOGIN_REQ;

enum DeviceType{
    XiaoAnBao1 = 1,
    XiaoAnBao2 = 2,
    XiaoAnBao3 = 3,
    XiaoAnBao4 = 4
};

typedef MSG_HEADER MSG_LOGIN_RSP;

/*
 * GPS structure
 */
typedef struct
{
    int timestamp;
    float longitude;
    float latitude;
    char speed;
    short course;
}__attribute__((__packed__)) GPS;

/*
 * GPS message structure
 */
typedef struct
{
    MSG_HEADER header;
    GPS gps;
}__attribute__((__packed__)) MSG_GPS;

/*
 * CELL structure
 */
typedef struct
{
   short lac;       //local area code
   short cellid;    //cell id
   short rxl;       //receive level
}__attribute__((__packed__)) CELL;

typedef struct
{
    short mcc;  //mobile country code
    short mnc;  //mobile network code
    char  cellNo;// cell count
    //CELL cell[];
}__attribute__((__packed__)) CGI;       //Cell Global Identifier

/*
 * CGI message structure
 */
 typedef struct
 {
     MSG_HEADER header;
     CGI cgi;
 }__attribute__((__packed__)) MSG_CGI;

/*
 * heartbeat message structure
 */
typedef struct
{
    MSG_HEADER header;
    short status;   //TODO: to define the status bits
}__attribute__((__packed__)) MSG_PING_REQ;

typedef MSG_HEADER MSG_PING_RSP;

/*
 * alarm message structure
 */
enum ALARM_TYPE
{
    ALARM_FENCE_OUT = 1,
    ALARM_FENCE_IN,
    ALARM_VIBRATE,
};

typedef struct
{
    MSG_HEADER header;
    char alarmType;
}__attribute__((__packed__)) MSG_ALARM_REQ;

typedef MSG_HEADER MSG_ALARM_RSP;

/*
 * SMS message structure
 */
typedef struct
{
    MSG_HEADER header;
    char telphone[TEL_NO_LENGTH + 1];
    char smsLen;
    char sms[];
}__attribute__((__packed__)) MSG_SMS_REQ;

typedef MSG_SMS_REQ MSG_SMS_RSP;

/*
 * seek message structure
 * the message has no response
 */
typedef struct
{
    MSG_HEADER header;
    int intensity;
}__attribute__((__packed__)) MSG_433;

/*
 * GPS set_time message structure
 */

typedef struct
{
    MSG_HEADER header;
    int timer;
}__attribute__((__packed__)) MSG_SET_TIMER_REQ;

typedef struct
{
    MSG_HEADER header;
    int result;
}__attribute__((__packed__)) MSG_SET_TIMER_RSP;

/*
*server set_ip/domain message structure
*this message has no response
*/
typedef struct
{
    MSG_HEADER header;
    int port;
    char server[];
}__attribute__((__packed__)) MSG_SET_SERVER;

/*
 * defend message structure
 */
 enum DEFEND_TYPE
{
    DEFEND_ON   = 0x01,
    DEFEND_OFF  = 0x02,
    DEFEND_GET  = 0x03,
};

/*
 * defend switch message structure
 */
typedef struct
{
    MSG_HEADER header;
    char result;
}__attribute__((__packed__)) MSG_DEFEND_ON_RSP;

typedef struct
{
    MSG_HEADER header;
    char result;
}__attribute__((__packed__)) MSG_DEFEND_OFF_RSP;

typedef struct
{
    MSG_HEADER header;
    char status;             //0: OFF,1: ON
}__attribute__((__packed__)) MSG_DEFEND_GET_RSP;


/*
 * switch on the seek mode
 */
 enum SEEK_TYPE
{
    SEEK_OFF    = 0x01,
    SEEK_ON     = 0x02,
};

typedef struct
{
    MSG_HEADER header;
    int token;
    char operator;     //refer to SEEK_TYPE
}__attribute__((__packed__)) MSG_SEEK_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    char result;
}__attribute__((__packed__)) MSG_SEEK_RSP;

typedef MSG_HEADER MSG_LOCATE;

typedef struct
{
    MSG_HEADER header;
    char isGps;
    GPS gps;
}__attribute__((__packed__)) MSG_GPSLOCATION_RSP;   //FIXME: change the name

typedef struct
{
    MSG_HEADER header;
    char isGps;
}__attribute__((__packed__)) MSG_CELLLOCATION_HEADER;   //FIXME: change the name



enum AUTODEFEND_SWITCH
{
    AUTO_DEFEND_OFF,
    AUTO_DEFEND_ON,
};

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char onOff; //refer to AUTODEFEND_SWITCH
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_SET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char result;
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_SET_RSP;

typedef struct
{
    MSG_HEADER header;
    int token;
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_GET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char result;
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_GET_RSP;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char period;   //time unit: minutes
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_SET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char result;
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_SET_RSP;


typedef struct
{
    MSG_HEADER header;
    int token;
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_GET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char period;   //time unit: minutes
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_GET_RSP;

typedef struct
{
    MSG_HEADER header;
    int starttime;
    int endtime;
    int mileage;
}__attribute__((__packed__)) MSG_ITINERARY_REQ;

typedef struct
{
    MSG_HEADER header;
    char percent;
    char miles;
}__attribute__((__packed__)) MSG_BATTERY_RSP;


typedef struct
{
    MSG_HEADER header;
    char state;             //0 express OFF,1 express ON
}__attribute__((__packed__)) MSG_AUTODEFEND_STATE_REQ;

typedef struct
{
    MSG_HEADER header;
    int version;
    int size;
}__attribute__((__packed__)) MSG_UPGRADE_START;

typedef struct
{
    MSG_HEADER header;
    char code;      //MSG_SUCCESS means OK to upgrade
}__attribute__((__packed__)) MSG_UPGRADE_START_RSP;

typedef struct
{
    MSG_HEADER header;
    int offset;
    char data[];
}__attribute__((__packed__)) MSG_UPGRADE_DATA;

typedef struct
{
    MSG_HEADER header;
    int offset;
}__attribute__((__packed__)) MSG_UPGRADE_DATA_RSP;

typedef struct
{
    MSG_HEADER header;
    int checksum;
    int size;
}__attribute__((__packed__)) MSG_UPGRADE_END;

typedef struct
{
    MSG_HEADER header;
    char code;
}__attribute__((__packed__)) MSG_UPGRADE_END_RSP;

typedef struct
{
    MSG_HEADER header;
    char CCID[MAX_CCID_LENGTH];
    char IMSI[IMSI_LENGTH];
}__attribute__((__packed__)) MSG_SIM_INFO;

typedef MSG_HEADER MSG_DEVICE_INFO_GET_REQ;
#pragma anon_unions
typedef struct
{
    MSG_HEADER header;

    //auto lock related configuration
    struct
    {
        char autolock;
        char autoperiod;
    }__attribute__((__packed__));

    //battery configuration
    struct
    {
        char percent;
        char miles;
    }__attribute__((__packed__));

    //defend switch status
    char defend;

    char isGps;
    union
    {
        GPS gps;        //GPS
        struct          //CELL
        {
            short mcc;
            short mnc;
            short lac;
            short cid;
        }__attribute__((__packed__));
    }__attribute__((__packed__));

}__attribute__((__packed__)) MSG_DEVICE_INFO_GET_RSP;

/*
 * packed GPS message structure
 */
typedef struct
{
    MSG_HEADER header;
    GPS gps[];
}__attribute__((__packed__)) MSG_GPS_PACK;


#pragma pack(pop)

#endif /* _PROTOCOL_H_ */
