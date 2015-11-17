#include "stm32f10x.h"
#include "stm32f10x_GPIO.h"

void LED_Configure(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;  
#ifdef STM32_M3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits (GPIOC, GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5);	
#else
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_SetBits (GPIOF, GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9);	
#endif
}


void LED_Toggle(int n) {
#ifdef STM32_M3
	if (n < 0 || n > 2) return;

	if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_3 << n)) {
		GPIO_ResetBits(GPIOC, GPIO_Pin_3 << n);
	} else {
		GPIO_SetBits(GPIOC, GPIO_Pin_3 << n);
	}
#else
	if (n < 0 || n > 3) return;

	if (GPIO_ReadOutputDataBit(GPIOF, GPIO_Pin_6 << n)) {
		GPIO_ResetBits(GPIOF, GPIO_Pin_6 << n);
	} else {
		GPIO_SetBits(GPIOF, GPIO_Pin_6 << n);
	}
#endif
}
