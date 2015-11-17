#include "devices.h"
#include "stm32f10x.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_adc.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "weighingsensor.h"
#include "UltrasonicWave.h"
#include "PauseSensor.h"
#include "zytemptn9.h"



typedef struct{
	U8 ver;
	U8 dev;
	U8 cmd;
	U8 len;
}Pro_Head_t;

typedef union{
	U32 v;
	U32 *p;
}Pro_Data_t;

typedef struct {
	Pro_Head_t head;
	Pro_Data_t dat;
}Protocol_t;

typedef struct{
	const char *str;
	uint32_t id;
} DevTable_t,CmdTable_t;

typedef struct {
	uint8_t dev;
	void (*func)(Pro_Head_t *pHead);
} DevCmdHandlerMap;

#define TASK_STACK_SIZE 256
#define TASK_IDLE_PRIORITY tskIDLE_PRIORITY + 1

#define __sendHandle(dat,size) USBCDC_senddata(dat,size)

static
void __handler_UltrasonicWave(Pro_Head_t *pHead)
{
	switch(pHead->cmd){
	case CMD_START:{
		UltrasonicWave_Start();
		}break;
	case CMD_STOP:{
		UltrasonicWave_Stop();
		}break;
	case CMD_RESET:{
		}break;
	}
}

static
void __handler_WeighingSensor(Pro_Head_t *pHead)
{
	switch(pHead->cmd){
	case CMD_START:{
		WeighingSensor_Start();
		}break;
	case CMD_STOP:{
		WeighingSensor_Stop();
		}break;
	}
}

static
void __handler_ZytempTN9(Pro_Head_t *pHead)
{
	switch(pHead->cmd){
	case CMD_START:{
		ZytempTN9_Start();
		}break;
	case CMD_STOP:{
		ZytempTN9_Stop();
		}break;
	case CMD_RESET:{
		if(pHead->len >= sizeof(Protocol_t)){
			uint32_t dat = *(uint32_t *)&pHead[1];
			ZytempTN9_SetRatio(dat);
		}
		}break;
	}
}

static
void __handler_MLX90614(Pro_Head_t *pHead)
{
	switch(pHead->cmd){
	case CMD_START:{
		MLX9061x_Start();
		}break;
	case CMD_STOP:{
		MLX9061x_Stop();
		}break;
	}
}

static
void __handler_PauseSensor(Pro_Head_t *pHead)
{
	switch(pHead->cmd){
	case CMD_START:{
		PauseSensor_Start();
		}break;
	case CMD_STOP:{
		PauseSensor_Stop();
		}break;
	}
}

static
void __handler_StepMotor(Pro_Head_t *pHead)
{
	switch(pHead->cmd){
	case CMD_MOTOR_FORW:{
		if(pHead->len >= sizeof(Protocol_t)){
			StepMotor_Forw(((Protocol_t *)pHead)->dat.v);
		}
		}break;
	case CMD_MOTOR_BACK:{
		if(pHead->len >= sizeof(Protocol_t)){
			StepMotor_Back(((Protocol_t *)pHead)->dat.v);
		}
		}break;
	case CMD_STOP:{
		StepMotor_Stop();
		}break;
	}
}

static const DevTable_t __devTables[]={
	{"height",	DEV_STATURE 	},
	{"weigh",	DEV_WEIGHT		},
	{"temp",	DEV_MLX			},
	{"heart",	DEV_PAUSE		},
	{"motor",	DEV_PAUSE		},
	{"all", 	DEV_ALL 		},
	{NULL,		NULL			},
};

int __get_dev_id(const char *dat, int size)
{
	const DevTable_t *it;
	for(it = __devTables; it->str; ++it){
		if(0 == strncmp(dat, it->str, strlen(it->str))){
			return it->id;
		}
	}
	return 0;
}

const char * __get_dev_str(DEV_ID dev)
{
	const DevTable_t *it;
	for(it = __devTables; it->str; ++it){
		if(dev == it->id) return it->str;
	}
	return 0;
}

int __get_cmd_id(const char *dat, int size)
{
	static const CmdTable_t cmdTables[] = {
		{"start",	CMD_START		},
		{"stop",	CMD_STOP		},
		{"reset",	CMD_RESET		},
		{"tamb",	CMD_HEIGHT_TAMB	},
		{"forw",	CMD_MOTOR_FORW	},
		{"back",	CMD_MOTOR_BACK	},
		{NULL,		NULL			},
	};
	const CmdTable_t *it;
	int i;
	for(i = 0; i < size;){
		if(dat[i++] == ':') break;
	}
	if(i >= size) return 0;
	for(it = cmdTables; it->str; ++it){
		if(0 == strncmp(&dat[i], it->str, strlen(it->str))){
			return it->id;
		}
	}
	return 0;
}

int __get_value(const char *dat, int size)
{
	#define DEC_MAX_LEN 10
	int i;
	int len;
	int ret = 0;
	char buffer[DEC_MAX_LEN + 1] = {0};
	for(i = 0; i < size;){
		if(dat[i++] == '=') break;
	}
	if(i >= size) return 0;
	len = size - i;
	if(len > DEC_MAX_LEN) return 0;
	memcpy(buffer, &dat[i], len);
	ret = atoi(buffer);
	return ret;
}

void DeviceHandlerCommand(const char* buf, uint16_t size)
{
	static const DevCmdHandlerMap handlerMaps[] = {
		{DEV_STATURE , __handler_UltrasonicWave	},
		{DEV_WEIGHT, __handler_WeighingSensor	},
		{DEV_ZYTEMP, __handler_ZytempTN9		},
		{DEV_PAUSE, __handler_PauseSensor		},
		{DEV_MOTOR, __handler_StepMotor			},
		{DEV_MLX,	__handler_MLX90614			},
		{0, NULL},
	};
	const DevCmdHandlerMap *map;
	Protocol_t pro;
	Pro_Head_t *pHead = (Pro_Head_t *)&pro;
	pHead->ver = 0;
	pHead->dev = __get_dev_id(buf, size);
	pHead->cmd = __get_cmd_id(buf, size);
	pHead->len = sizeof(Protocol_t);
	pro.dat.v = __get_value(buf, size);
	
	for (map = handlerMaps; map->func; ++map) {
		if(pHead->dev == DEV_ALL){
			map->func(pHead);
		}else if(pHead->dev == map->dev) {
			map->func(pHead);
			break;
		}
	}
}

void Dev_SendData(DEV_ID dev, uint32_t dat,bool inISR)
{
	Protocol_t *pro = (Protocol_t *)pvPortMalloc(sizeof(Protocol_t));
	Pro_Head_t *pHead = (Pro_Head_t *)pro;
	pHead->cmd = CMD_DEV_BASE;
	pHead->dev = dev;
	pHead->ver = 0;
	pro->dat.v = dat;
	pHead->len = sizeof(Protocol_t);
	Comm_SendToUsb((char *)pro, pHead->len, inISR);
	vPortFree(pro);
}

void Dev_SendIntWithType(DEV_ID dev, uint32_t dat,const char *type, bool inISR)
{
	#define MAX_BUF_SIZE	128
	char *buffer;
	int len;
	const char *str = __get_dev_str(dev);
	if(str == 0) return;
	buffer = pvPortMalloc(MAX_BUF_SIZE);
	len = sprintf(buffer, "%s:%s=%d", str, type, dat);
	Comm_SendToUsb(buffer, len, inISR);
	vPortFree(buffer);
}

void Dev_SendIntData(DEV_ID dev, uint32_t dat, bool inISR)
{
	Dev_SendIntWithType(dev, dat, "value", inISR);
}

void Devices_Init() {
	UltrasonicWave_Config();
	WeighingSensor_Config();
	ZytempTN9_Config();
	PauseSensor_Config();
	StepMotor_Config();
	MLX9061x_Config();
}

