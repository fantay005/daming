/*
 * 土壤湿度: 	ADC		(PA5,ADC12_INT5)
 * 土壤温度:	DS18B20	(PA4)
 */
#include "stdbool.h"
#include "soil.h"
#include "timdelay.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_dma.h"
#include "misc.h"
#include "adc_dma.h"
#include "onewire.h"

#define GPIOx	GPIOE
#define PINx_t1	GPIO_Pin_8
// #define PINx_t2	GPIO_Pin_3
// #define PINx_t3	GPIO_Pin_4

#define TEMPLATE_SET(io, pin, b)	\
	do{								\
		if(b){						\
			GPIO_SetBits(io, pin);	\
		}else{						\
			GPIO_ResetBits(io, pin);\
		}							\
	}while(0)

#define TEMPLATE_GET(io, pin)		\
	do{								\
	return GPIO_ReadInputDataBit(io, pin);\
	}while(0)

static
void __st1_set(bool b)
{
	TEMPLATE_SET(GPIOx, PINx_t1, b);
}

static
uint8_t __st1_get(void)
{
	TEMPLATE_GET(GPIOx, PINx_t1);
}

// static
// void __st2_set(bool b)
// {
// 	TEMPLATE_SET(GPIOx, PINx_t2, b);
// }

// static
// uint8_t __st2_get(void)
// {
// 	TEMPLATE_GET(GPIOx, PINx_t2);
// }

// static
// void __st3_set(bool b)
// {
// 	TEMPLATE_SET(GPIOx, PINx_t3, b);
// }

// static
// uint8_t __st3_get(void)
// {
// 	TEMPLATE_GET(GPIOx, PINx_t3);
// }

static One_Wire_t __DS18B20s[] = {
	{
		__st1_set,
		__st1_get,
		TimDelay_Us
	},
// 	{
// 		__st2_set,
// 		__st2_get,
// 		TimDelay_Us
// 	},
// 	{
// 		__st3_set,
// 		__st3_get,
// 		TimDelay_Us
// 	},
};

static
void __DS_GPIO_Config(void)
{
	/*GPIO*/{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = PINx_t1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStructure);
	}
	GPIO_SetBits(GPIOx, PINx_t1);
}

void SoilSensor_Config(void)
{
	__DS_GPIO_Config();
}

static
uint32_t __ds18b20_read(One_Wire_t *ow)
{
	uint8_t lsb, msb, s = 0;
	int temp;
	OneWire_Reset(ow);
	OneWire_Write(ow,0xCC);
	OneWire_Write(ow,0x44);
	vTaskDelay(configTICK_RATE_HZ-100);
	OneWire_Reset(ow);
	OneWire_Write(ow,0xCC);
	OneWire_Write(ow,0xBE);
	lsb = OneWire_read(ow);
	msb = OneWire_read(ow);
	s = msb & 0x80?1:0;
	msb = (msb<<4)|(lsb>>4);
	if(s){
		msb = ~msb;
		lsb = ~lsb + 1;
	}
	temp = msb*1000 + (lsb&0xf)*62.5;
	if(s){
		temp = 0 - temp;
	}
	return temp;
}

uint32_t SoilTemp_Read(uint32_t index)
{
	if(index >= (sizeof(__DS18B20s)/sizeof(__DS18B20s[0]))){
		return 0;
	}
	return __ds18b20_read(&__DS18B20s[index]);
}

/*
 * 湿度计算公式: 计算结果电压值/2000mV (%)
 */
uint32_t SoilHum_Read(uint32_t index)
{//计算结果(mV) = retVal *3300/4096
	index += ADC_ADDR_HUM_FIRST;
	if((index < ADC_ADDR_HUM_FIRST) || (index > ADC_ADDR_HUM_LAST))
			return 0;
	return ADCx_Read(index);
}

