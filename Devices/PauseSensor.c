#include "pausesensor.h"
#include "stm32f10x.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_RCC.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "misc.h"
#include <stdbool.h>

#define SAMPLE_RATE 200
#define FILETER_NUM 10
#define SAMPLE_MIDDLE  2048

#define GPIOx	GPIOA
#define PINx	GPIO_Pin_4
#define ADCx	ADC1
#define TIMx	TIM3

#define __SendData(dat)		Dev_SendIntData(DEV_PAUSE,(dat),IN_ISR)

static void __ADC1_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable ADCx and GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
	/* Configure PC.01  as analog input */
	GPIO_InitStructure.GPIO_Pin = PINx;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOx, &GPIO_InitStructure);				// PC1,输入时不用设置速率
}

static
void __Tim3_Init(void)
{
	/*配置定时器中断*/{
    NVIC_InitTypeDef NVIC_InitStructure; 
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	}
	/*配置定时器*/{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);
    TIM_DeInit(TIMx);
    TIM_TimeBaseStructure.TIM_Period= 1000000/SAMPLE_RATE - 1;		 								/* 自动重装载寄存器周期的值(计数值) */
    TIM_TimeBaseStructure.TIM_Prescaler= (72 - 1);				                /* 时钟预分频数 72M/360 */
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 		                /* 采样分频 */
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Down;                   /* 向上计数模式 */
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIMx, ENABLE);
    TIM_ClearFlag(TIMx, TIM_FLAG_Update);	
	}
	TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE);
}


static void __ADC1_Mode_Config(void)
{
	ADC_InitTypeDef ADC_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure; 
	/* ADCx configuration */
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//独立ADC模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE ; 	 //禁止扫描模式，扫描模式用于多通道采集
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//不用连续转换
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//不使用外部触发转换
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 	//采集数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;	 	//要转换的通道数目1
	ADC_Init(ADCx, &ADC_InitStructure);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
	ADC_RegularChannelConfig(ADCx, ADC_Channel_4, 1, ADC_SampleTime_55Cycles5);
		
	ADC_Cmd(ADCx, ENABLE);
	
	/*复位校准寄存器 */   
	ADC_ResetCalibration(ADCx);
	/*等待校准寄存器复位完成 */
	while(ADC_GetResetCalibrationStatus(ADCx));
	
	/* ADC校准 */
	ADC_StartCalibration(ADCx);
	ADC_ITConfig(ADCx, ADC_IT_EOC, ENABLE);
	/* 等待校准完成*/
	while(ADC_GetCalibrationStatus(ADCx));

		/*配置定时器中断*/
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

static
void __complete_measure(int heart)
{
	__SendData(heart);
}

static uint16_t __filter(uint16_t dat) {
	static uint32_t preValues[FILETER_NUM] = {0};
	static uint32_t curFilterIndex = 0;
	uint32_t sum = 0;
	uint32_t i;

	preValues[curFilterIndex] = dat;
	curFilterIndex++;
	if (curFilterIndex >= FILETER_NUM) {
		curFilterIndex = 0;
	}

	for (i = 0; i < FILETER_NUM; ++i) {
		sum += preValues[i];
	}
	return sum / FILETER_NUM;
}

static void __onDataReady(uint16_t dat) {
	static uint16_t index = 0;
	static uint16_t preMaxValue = 0;
	static uint16_t maxValue = 0;
	static uint16_t maxIndex = 0;
	static uint16_t downCount = 0;
	static bool thisMaxIsConfirmed = false;
	dat = __filter(dat);
	++index;

	if (dat > maxValue) {
		maxValue = dat;
		maxIndex = index;
		thisMaxIsConfirmed = false;
		downCount = 0;
	} else if (dat < SAMPLE_MIDDLE) {
		++downCount;
		if ((!thisMaxIsConfirmed) && (downCount >= SAMPLE_RATE/5)) {
			uint32_t heartbeat = 60*SAMPLE_RATE / (maxIndex);
			__complete_measure(heartbeat);
//			printf("H");
			index -= maxIndex;
			preMaxValue = maxValue;
			maxValue = SAMPLE_MIDDLE;
			thisMaxIsConfirmed = true;
			downCount = 0;
		}
	}
}

void PauseSensor_Start(void)
{
	TIM_Cmd(TIMx, ENABLE);
}
void PauseSensor_Stop(void)
{
	TIM_Cmd(TIMx, DISABLE);
}

void PauseSensor_Config(void)
{
	__ADC1_GPIO_Config();
	__ADC1_Mode_Config();
	__Tim3_Init();
}

void TIM3_IRQHandler(void)
{
	if ( TIM_GetITStatus(TIMx , TIM_IT_Update) != RESET ){	
		TIM_ClearITPendingBit(TIMx , TIM_IT_Update); 
		ADC_SoftwareStartConvCmd(ADCx, ENABLE);
	} 	
}

#if 0
void ADC1_2_IRQHandler(void)
{
	if(ADC_GetITStatus(ADCx, ADC_IT_EOC) != RESET){
		int dat = ADC_GetConversionValue(ADCx);
		__onDataReady(dat);
		ADC_ClearITPendingBit(ADCx, ADC_IT_EOC);
	}
}
#endif

