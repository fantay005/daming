/*
 * 光照传感器
 * POWERON	0x01	传感器上电
 * MESA		0x11	传感器测量命令
 *			延时120ms后读该地址
 *			计算结果(dat[0]<<8 + dat[1])/(1.2*2)
 */
#include "BH1750.h"
#include "softi2c.h"
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "misc.h"

#define GPIOx		GPIOE
#define PINx_SDA	GPIO_Pin_15
#define PINx_SCL	GPIO_Pin_14

#define ADDR		0x46
#define CMD_PWR		0x01
#define CMD_MES		0x10                                      

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
	100,
};

void BH1750_PowerON(void)
{
	int reTry = 100;
	while(--reTry){
		SoftI2C_Stop(&__SoftI2C);
		vTaskDelay(configTICK_RATE_HZ/100);
		SoftI2C_Start(&__SoftI2C);
		if(!SoftI2C_Write(&__SoftI2C, ADDR)) continue;
		if(!SoftI2C_Write(&__SoftI2C, CMD_PWR)) continue;
		SoftI2C_Stop(&__SoftI2C);
		break;
	}
}

uint32_t BH1750_Read(void)
{//计算结果 = retVal/2.4
	uint8_t dat[2] = {0};
	int reTry = 100;
	while(--reTry){
		SoftI2C_Stop(&__SoftI2C);
		SoftI2C_Start(&__SoftI2C);
		if(!SoftI2C_Write(&__SoftI2C, ADDR)) continue;
		if(!SoftI2C_Write(&__SoftI2C, CMD_MES)) continue;
		SoftI2C_Stop(&__SoftI2C);
		vTaskDelay(configTICK_RATE_HZ/5);
		
		SoftI2C_Start(&__SoftI2C);
		if(!SoftI2C_Write(&__SoftI2C, ADDR|0x01)) continue;
		dat[0] = SoftI2C_Read(&__SoftI2C,1);
		dat[1] = SoftI2C_Read(&__SoftI2C,0);	
		SoftI2C_Stop(&__SoftI2C);
		break;
	}
	return (dat[0]<<8 | dat[1]);
}

void BH1750_config(void)
{
	/*配置GPIO*/{
	GPIO_InitTypeDef mGPIO_InitTypeDef;
	
	mGPIO_InitTypeDef.GPIO_Mode = GPIO_Mode_Out_OD;
	mGPIO_InitTypeDef.GPIO_Pin = PINx_SCL | PINx_SDA;
	mGPIO_InitTypeDef.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOx, &mGPIO_InitTypeDef);
	}	
	
	SoftI2C_Stop(&__SoftI2C);
	BH1750_PowerON();
}


