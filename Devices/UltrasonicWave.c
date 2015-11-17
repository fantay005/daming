#include "UltrasonicWave.h"
#include "stm32f10x.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_RCC.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "FreeRtos.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>

#define	GPIOx      	GPIOA      
#define	PINx_Trig   GPIO_Pin_0   //TRIG       
#define	PINx_Echo   GPIO_Pin_1	 //ECHO   

#define TIMx			TIM2
#define TIMx_IRQHandler	TIMx_IRQHandler
#define TIMx_IRQn		TIM2_IRQn

#define FOSC				72000000UL
#define SAMPLE_RATE 		1000000UL
#define TASK_STACK_SIZE 	256
#define TASK_IDLE_PRIORITY	tskIDLE_PRIORITY + 1
#define TASK_NAME 			"Height"

typedef struct{
	uint32_t type;
	union{
		uint32_t v;
		void *p;
	}dat;
}qCmd_t;

static xQueueHandle __qCmd	= 0;
static uint16_t __sspeed 	= 340;		//时声速为340米/秒


#define CALC(val,speed)		(((val)*(speed)*1000/SAMPLE_RATE)/2)
#define __SendData(dat)	Dev_SendIntData(DEV_STATURE,(dat), IN_TASK)

static
uint16_t __get_sonic_speed(float temp)
{
	struct {
		short temp;
		U16 vs;
	}static const vs_table[]={
		{-10,325},
		{-5,328},
		{0,331},
		{5,334},
		{10,337},
		{15,340},
		{20,343},
		{25,346},
		{30,349},
		{35,351},
		{40,354},
		{45,357},
		{50,360},
	};
	int i;
	for(i = 0; i < sizeof(vs_table)/sizeof(vs_table[0]); i++){
		if(temp >= vs_table[i].temp) return vs_table[i].vs;
	}
	return 340;
}
static
void __delay(int Time)    
{
   unsigned char i;
   for ( ; Time>0; Time--)
     for ( i = 0; i < 72; i++ );
}

static void __task(void *nouse) {
	static bool run = false;
	static bool sequence = 5;

	qCmd_t cmd;
	__qCmd = xQueueCreate(5, sizeof(qCmd_t));
	while(1) {
		if (pdTRUE == xQueueReceive(__qCmd,&cmd, configTICK_RATE_HZ/sequence)) {
			if(cmd.type == CMD_START){
				run = true;
			}else if(cmd.type == CMD_STOP){
				run = false;
			}else if(cmd.type == CMD_COMPLETE){
				uint32_t data = CALC(cmd.dat.v, __sspeed);
				__SendData(data);
			}else if(cmd.type == CMD_HEIGHT_TAMB){
				float *p = cmd.dat.p;
				__sspeed = __get_sonic_speed(*p);
			}
		} 
		if (run) {
			GPIO_SetBits(GPIOx,PINx_Trig);	//送>10US的高电平
			__delay(20);					//延时20US
			GPIO_ResetBits(GPIOx,PINx_Trig);
		}
		vTaskDelay(configTICK_RATE_HZ);
	}
}

static
void __Tim2_Init(void)
{
	/*配置定时器中断*/{
    NVIC_InitTypeDef NVIC_InitStructure; 
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
    NVIC_InitStructure.NVIC_IRQChannel = TIMx_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	}
	/*配置定时器*/{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);
    TIM_DeInit(TIMx);
    TIM_TimeBaseStructure.TIM_Period=65535;		 								/* 自动重装载寄存器周期的值(计数值) */
    TIM_TimeBaseStructure.TIM_Prescaler= (FOSC/SAMPLE_RATE - 1);				                /* 时钟预分频数 72M/360 */
    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 		                /* 采样分频 */
    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;                   /* 向上计数模式 */
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
    TIM_ClearFlag(TIMx, TIM_FLAG_Update);	
	}
	/*配置通道2为PWM输入模式*/{
	TIM_ICInitTypeDef  TIM_ICInitStructure;
	
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_PWMIConfig(TIMx, &TIM_ICInitStructure);
	
	TIM_SelectInputTrigger(TIMx, TIM_TS_TI2FP2);
	TIM_SelectSlaveMode(TIMx, TIM_SlaveMode_Reset);
	TIM_SelectMasterSlaveMode(TIMx, TIM_MasterSlaveMode_Enable);
	TIM_ITConfig(TIMx, TIM_IT_CC2, ENABLE);
	}
	TIM_Cmd(TIMx, ENABLE);
}

static
void __complete_measure(uint32_t dat)
{
	qCmd_t cmd;
	signed portBASE_TYPE isNeedSwitchTask = 0;
	cmd.type = CMD_COMPLETE;
	cmd.dat.v = dat;
	if(pdTRUE == xQueueSendFromISR(__qCmd, &cmd, &isNeedSwitchTask)){
		if(isNeedSwitchTask){
			portYIELD();
		}
		return;
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

void UltrasonicWave_Start()
{
	__control(CMD_START,0);
}
void UltrasonicWave_Stop()
{
	__control(CMD_STOP,0);
}

void UltrasonicWave_SetTamb(float tamb)
{
	static float temp = 15.0;
	temp = tamb;
	__control(CMD_HEIGHT_TAMB, (uint32_t )&temp);
}

void UltrasonicWave_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Pin = PINx_Trig; 				   //PC8接TRIG
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		   //设为推挽输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		   
	GPIO_Init(GPIOx, &GPIO_InitStructure);					   //初始化外设GPIO 

	GPIO_InitStructure.GPIO_Pin = PINx_Echo;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOx, &GPIO_InitStructure);
	__Tim2_Init();
	
    xTaskCreate(__task, (signed portCHAR *)TASK_NAME, 
                TASK_STACK_SIZE, NULL, TASK_IDLE_PRIORITY, NULL);
}

void TIMx_IRQHandler(void)
{
	uint16_t IC1Value;
	if ( TIM_GetITStatus(TIMx , TIM_IT_Update) != RESET ){	
		TIM_ClearITPendingBit(TIMx , TIM_IT_Update); 
	}else if( TIM_GetITStatus(TIMx, TIM_IT_CC2) != RESET ){
		TIM_ClearITPendingBit(TIMx, TIM_IT_CC2);
		IC1Value = TIM_GetCapture1(TIMx);
		__complete_measure(IC1Value);
	}	 	
}

