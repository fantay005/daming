#include "stepmotor.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "devices.h"

#define STEPMOTOR_TASK_STACK_SIZE 256
#define TIMx 		TIM4
#define RCC_TIMx	RCC_APB1Periph_TIM4
#define IRQ_TIMx	TIM4_IRQn
#define GPIOx		GPIOC
#define PINx_Int1	GPIO_Pin_6
#define PINx_Int2	GPIO_Pin_7
#define PINx_Int3	GPIO_Pin_8
#define PINx_Int4	GPIO_Pin_9


#if 1
static uint16_t steps[8] = {PINx_Int1, 
							PINx_Int1|PINx_Int2, 
							PINx_Int2,
							PINx_Int2|PINx_Int3, 
				   			PINx_Int3,
				   			PINx_Int3|PINx_Int4,
				   			PINx_Int4, 
				   			PINx_Int4|PINx_Int1};
#else
static uint16_t steps[4] = {0x01<<PINx_Offset, 
							0x02<<PINx_Offset,
							0x04<<PINx_Offset,
							0x08<<PINx_Offset};
#endif

#define QSize_StepMotor	32
static xQueueHandle __qStepMotor;
static int moveStepNum = 0;
static int curPosition = 0;
static int maxPosition = 0xffff;

static
void __set_port(uint16_t dat)
{
	uint16_t port;
	
	port = GPIO_ReadOutputData(GPIOx);
	port &= ~(PINx_Int1|PINx_Int2|PINx_Int3|PINx_Int4);
	port |= dat;
	GPIO_Write(GPIOx,port);
}

static
void __start(void)
{
	TIM_Cmd(TIMx, ENABLE);
}
static
void __stop(void)
{
	TIM_Cmd(TIMx, DISABLE);
}

static
void __move_abs(int pos)
{
	__stop();
	moveStepNum = pos - curPosition;
	__start();
}

static
void __move_ref(int offset)
{
	__stop();
	moveStepNum = offset;
	__start();
}

static
void __move_step(void)
{
	static int i = 0;
	if(moveStepNum > 0){
		i++;
		if(i >= sizeof(steps)/sizeof(steps[0])) i = 0;
		__set_port(steps[i]);
		moveStepNum--;
		curPosition++;
	}else if(moveStepNum < 0){
		i--;
		if(i < 0) i = sizeof(steps)/sizeof(steps[0]) - 1;
		__set_port(steps[i]);
		moveStepNum++;
		curPosition--;
	}else{
		__stop();
	}
}

void __task(void *nouse)
{
	while(1){
		vTaskDelay(configTICK_RATE_HZ/5);
	}
}

void StepMotor_Forw(uint32_t step)
{
	__move_ref(step);
}

void StepMotor_Back(uint32_t step)
{
	__move_ref(0 - step);
}

void StepMotor_Stop()
{
	__stop();
}
void StepMotor_Config(void)
{
	RCC_APB1PeriphClockCmd(RCC_TIMx, ENABLE);
	/* 配置gpio */{
	GPIO_InitTypeDef mGPIO_InitTypeDef;
	mGPIO_InitTypeDef.GPIO_Mode = GPIO_Mode_Out_PP;
	mGPIO_InitTypeDef.GPIO_Pin = PINx_Int1|PINx_Int2|PINx_Int3|PINx_Int4;
	mGPIO_InitTypeDef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &mGPIO_InitTypeDef);
	}

	/*配置中断向量*/{
	NVIC_InitTypeDef mNVIC_InitTypeDef;
	mNVIC_InitTypeDef.NVIC_IRQChannel = IRQ_TIMx;
	mNVIC_InitTypeDef.NVIC_IRQChannelSubPriority = 0;
	mNVIC_InitTypeDef.NVIC_IRQChannelPreemptionPriority = 0;
	mNVIC_InitTypeDef.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&mNVIC_InitTypeDef);
	}

	/*初始化定时器*/{
	TIM_TimeBaseInitTypeDef mTIM_TimeBaseInitTypeDef;
	TIM_TimeBaseStructInit(&mTIM_TimeBaseInitTypeDef);
	mTIM_TimeBaseInitTypeDef.TIM_CounterMode = TIM_CounterMode_Down;
	mTIM_TimeBaseInitTypeDef.TIM_Period = 1000 - 1;
	mTIM_TimeBaseInitTypeDef.TIM_Prescaler = (72 - 1);
	TIM_TimeBaseInit(TIMx, &mTIM_TimeBaseInitTypeDef);
	TIM_CCPreloadControl(TIMx, ENABLE);
	TIM_ClearFlag(TIMx, TIM_FLAG_Update);
	TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE);
	}
	TIM_Cmd(TIMx, DISABLE);
	xTaskCreate(__task, (signed portCHAR *)"Motor", STEPMOTOR_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
	StepMotor_Forw(400);
}

void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIMx, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIMx, TIM_IT_Update);
		__move_step();
	}
}

