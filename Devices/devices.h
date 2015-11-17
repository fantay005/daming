#ifndef __DEVICES_H__
#define __DEVICES_H__

#include "stm32f10x.h"
#include <stdbool.h>
#include "comm.h"

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;

#define CMD_DEV_BASE	100

typedef enum{
	DEV_STATURE = 1,
	DEV_WEIGHT,	
	DEV_ZYTEMP,
	DEV_PAUSE,
	DEV_MOTOR,
	DEV_MLX,
	DEV_ALL = 255,
}DEV_ID;

typedef enum {
	CMD_STOP = 0,
	CMD_START,
	CMD_RESET,
	CMD_COMPLETE = CMD_DEV_BASE,
	CMD_RAW = 255,
}CMD_ID;

typedef enum{
	CMD_MOTOR_BASE = CMD_DEV_BASE,
	CMD_MOTOR_FORW,
	CMD_MOTOR_BACK,
}CMD_MOTOR_ID;

typedef enum{
	CMD_HEIGHT_BASE = CMD_DEV_BASE,
	CMD_HEIGHT_TAMB,
}CMD_HEIGHT_ID;


void Dev_Init(void);
void Dev_SendIntData(DEV_ID dev, uint32_t dat,bool inISR);
void Dev_SendIntWithType(DEV_ID dev, uint32_t dat,const char *type,bool inISR);



#endif

