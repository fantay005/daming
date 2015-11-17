#include "weighingsensor.h"
#include "stm32f10x_GPIO.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>

#define USE_HX711

#ifdef USE_HX711
#define GPIOx	 GPIOA
#define PINx_Din GPIO_Pin_2
#define PINx_Sck GPIO_Pin_3

#define TASK_STACK_SIZE 256
#define TASK_IDLE_PRIORITY tskIDLE_PRIORITY + 1
#define TASK_NAME "Weight"
#define __SendData(dat)		Dev_SendIntData(DEV_WEIGHT,(dat),IN_TASK)

#define Din_Reset()		GPIO_ResetBits(GPIOx,PINx_Din)
#define Din_Set()	    GPIO_SetBits(GPIOx,PINx_Din)
#define Din_Get() 		GPIO_ReadInputDataBit(GPIOx,PINx_Din)
#define SCK_Set()		GPIO_SetBits(GPIOx, PINx_Sck)
#define SCK_Reset()		GPIO_ResetBits(GPIOx, PINx_Sck)
#define DelayUs(n)	do{int a; for(a = 0; a < (n); a++);}while(0)

#define QSize_DevCmd	32
static xQueueHandle __qCmd;

typedef struct{
	uint32_t type;
	union{
		uint32_t v;
		void *p;
	}dat;
}qCmd_t;

static
void __hx711_Start()
{//	DO:	--\_
	SCK_Reset();
	Din_Set();
}

#if 0
static
void __hx711_Stop()
{//	SCK:	_/-----
	SCK_Set();
	DelayUs(60);
}
#endif

static
uint32_t __hx711_Read(HX711_Mode_Sel sel)
{
	uint32_t dat = 0;
	int i = 0;
	__hx711_Start();
	while(Din_Get()){
		vTaskDelay(configTICK_RATE_HZ/500);
		if(i++ > 10) return 0;
	}	
	for(i = 23; i >= 0; i--){
		SCK_Set();	 DelayUs(1);
		SCK_Reset();
		if(Din_Get()) dat |= (1<<i);
		DelayUs(1);
	}

	i = (int)sel;
	do{				
		SCK_Set();	
		DelayUs(1);
		SCK_Reset();
		DelayUs(1);
	}while(i--);
	Din_Set();
	return dat;
}
#endif

static
void __complete_measure(uint32_t dat)
{
//	printf("W");
	__SendData(dat);
}
static
void __task(void *nouse)
{
	static bool run = false;
	static bool sequence = 5;
	static HX711_Mode_Sel mode = HX711_A_64;
	qCmd_t cmd;
	__qCmd = xQueueCreate(QSize_DevCmd, sizeof(qCmd_t));
	
	while(1){
		if(pdTRUE == xQueueReceive(__qCmd, &cmd, configTICK_RATE_HZ/sequence)){
			if(cmd.type == CMD_START){
				run = true;
			}else if(cmd.type == CMD_STOP){
				run = false;
			}
		}
		if(run){ 
			int dat = __hx711_Read(mode);
			__complete_measure(dat);   
		}
	}
}

static
void __control(uint32_t cmd, uint32_t dat)
{
	qCmd_t qcmd;
	qcmd.type = cmd;
	qcmd.dat.v = dat;
	if(pdTRUE == xQueueSend(__qCmd, &qcmd, configTICK_RATE_HZ/5)){
		return;
	}
}
void WeighingSensor_Start()
{
	__control(CMD_START, 0);
}
void WeighingSensor_Stop()
{
	__control(CMD_STOP, 0);
}

void WeighingSensor_Config(void)
{
#ifdef USE_HX711
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Pin = PINx_Din;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PINx_Sck;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStructure);
	
	Din_Set();
	SCK_Set();
    xTaskCreate(__task, (signed portCHAR *)TASK_NAME, 
                TASK_STACK_SIZE, NULL, TASK_IDLE_PRIORITY, NULL);

#else	
#endif	
}

