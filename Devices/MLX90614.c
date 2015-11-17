#include "mlx9061x.h"
#include "devices.h"
#include <stdbool.h>

#include "stm32f10x_I2C.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

#include "softi2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define TASK_STACK_SIZE 256
#define TASK_IDLE_PRIORITY tskIDLE_PRIORITY + 1
#define TASK_NAME "MLX"

#define GPIOx		GPIOC
#define PINx_SCL	GPIO_Pin_8
#define PINx_SDA	GPIO_Pin_9

#define SADD		(0x5A<<1)
#define TOBJ		(0x00|0x07)

#define __SendData(dat)	Dev_SendIntWithType(DEV_MLX, (dat),"90614", IN_TASK)

#define MAX_QUEUE_SIZE	32
static xQueueHandle __qCmd;

static
void __setSCL(bool isHigh)
{
	if(isHigh)
		GPIO_SetBits(GPIOx, PINx_SCL);
	else
		GPIO_ResetBits(GPIOx, PINx_SCL);
}

static
bool __getSCL(void)
{
	return GPIO_ReadInputDataBit(GPIOx, PINx_SCL);
}

static
void __setSDA(bool isHigh)
{
	if(isHigh)
		GPIO_SetBits(GPIOx, PINx_SDA);
	else
		GPIO_ResetBits(GPIOx, PINx_SDA);
}
static
bool __getSDA(void)
{
	return GPIO_ReadInputDataBit(GPIOx, PINx_SDA);
}

static
void __delayUs(int us)
{
	int i;
	for(i = 0; i < 3*us; i++);
}

const static SoftI2C_t __SoftI2C = {
	__setSCL,
	__getSCL,
	__setSDA,
	__getSDA,
	__delayUs,
	10,
};

static
void __task(void *nouse)
{
	static bool run = false;
	uint8_t cmd;
	__qCmd = xQueueCreate(MAX_QUEUE_SIZE, sizeof(uint8_t *));
	while(1){
		if(pdTRUE == xQueueReceive(__qCmd, &cmd, configTICK_RATE_HZ)){
			if(cmd == CMD_START){
				run = true;
			}else if(cmd == CMD_STOP){
				run = false;
			}
		}else if(run){
			uint16_t data = MLX9061x_memRead(&__SoftI2C, SADD, TOBJ);
			__SendData(data);
		}
	}
}

void MLX90614_Start(void)
{
	uint8_t cmd = CMD_START;
	xQueueSend(__qCmd, &cmd, configTICK_RATE_HZ);
}

void MLX90614_Stop(void)
{
	uint8_t cmd = CMD_STOP;
	xQueueSend(__qCmd, &cmd, configTICK_RATE_HZ);
}

void MLX90614_Config(void)
{
	/*ÅäÖÃÊ±ÖÓ*/{
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	/*ÅäÖÃGPIO*/{
	GPIO_InitTypeDef mGPIO_InitTypeDef;
	
	mGPIO_InitTypeDef.GPIO_Mode = GPIO_Mode_Out_OD;
	mGPIO_InitTypeDef.GPIO_Pin = PINx_SCL | PINx_SDA;
	mGPIO_InitTypeDef.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOx, &mGPIO_InitTypeDef);
	}

	xTaskCreate(__task, (signed portCHAR *)TASK_NAME, 
			TASK_STACK_SIZE, NULL, TASK_IDLE_PRIORITY, NULL);
	
}

