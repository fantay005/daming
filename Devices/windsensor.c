/*
 * 风传感器
 */
#include "windsensor.h"
#include "comm.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "adc_dma.h"


#define GPIOx			GPIOA
#define PINx			GPIO_Pin_1
#define TIMx			TIM2
#define TIMx_IRQHandler	TIM2_IRQHandler
#define TIMx_IRQn		TIM2_IRQn
#define FOSC			72000000UL
#define CYCLE		 	100000

void WindSensor_Config(void)
{
	/*GPIO*/{
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Pin = PINx;		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	   
	GPIO_Init(GPIOx, &GPIO_InitStructure);		
	}
	/*配置定时器*/{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
    TIM_DeInit(TIMx);
    TIM_TimeBaseStructure.TIM_Period=65535;		 								/* 自动重装载寄存器周期的值(计数值) */
    TIM_TimeBaseStructure.TIM_Prescaler= (FOSC/CYCLE - 1);				                /* 时钟预分频数 72M/360 */
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 		                /* 采样分频 */
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;                   /* 向上计数模式 */
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
    TIM_ClearFlag(TIMx, TIM_FLAG_Update);	
	}
	/*配置通道2为捕获模式*/{
	TIM_ICInitTypeDef  TIM_ICInitStructure;
	
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_PWMIConfig(TIMx, &TIM_ICInitStructure);
	
	TIM_SelectInputTrigger(TIMx, TIM_TS_TI2FP2);
	TIM_SelectSlaveMode(TIMx, TIM_SlaveMode_Reset);
	TIM_SelectMasterSlaveMode(TIMx, TIM_MasterSlaveMode_Enable);
	TIM_ITConfig(TIMx, TIM_IT_CC2|TIM_IT_Update, ENABLE);
	}
	/*配置定时器中断*/{
    NVIC_InitTypeDef NVIC_InitStructure; 
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
    NVIC_InitStructure.NVIC_IRQChannel = TIMx_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	}
	TIM_Cmd(TIMx, ENABLE);
}

static uint32_t speed;

/*
 * 风速计算: 10000/retVal m/s
 * 备注: retVal == 0 时表示没有风
 */
uint32_t WindSpeed_Read(void)
{//计算结果(10us): 
	uint32_t tmp = speed;
	speed = 0;
	return tmp;
}

/*
 * 风向计算结果: 电压值(mV)
 * 备注:使用100欧姆电阻做电流到电压的转换
 * 正北:400mV	东北~:500mV		东北:600mV	东~北:700mV
 * 正东:800mV	东~南:900mV		东南:1000mV	东南~:1100mV
 * 正南:1200mV	西南~:1300mV	西南:1400mV	西~南:1500mV
 * 正西:1600mV	西~北:1700mV	西北:1800mV	西北~:1900mV
 */
uint32_t WindDir_Read(void)
{//计算结果(mV) = retVal *3300/4096
	return ADCx_Read(ADC_ADDR_WINDDIR);
}

void TIMx_IRQHandler(void)
{
	if ( TIM_GetITStatus(TIMx , TIM_IT_Update) != RESET ){	
		TIM_ClearITPendingBit(TIMx , TIM_IT_Update); 
	}else if( TIM_GetITStatus(TIMx, TIM_IT_CC2) != RESET ){
		TIM_ClearITPendingBit(TIMx, TIM_IT_CC2);
		speed = TIM_GetCapture2(TIMx);
	}	 	
}


