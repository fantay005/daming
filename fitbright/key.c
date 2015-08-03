#include "key.h"
#include "stm32f10x_gpio.h"

#define KEY_PIN_0      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0) 
#define KEY_PIN_1      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)
#define KEY_PIN_2      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_2)
#define KEY_PIN_3      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_3)
#define KEY_PIN_4      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)
#define KEY_PIN_5      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)
#define KEY_PIN_6      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6)
#define KEY_PIN_7      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)
#define KEY_PIN_8      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_8)
#define KEY_PIN_9      GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9)
#define KEY_PIN_U      GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_0)
#define KEY_PIN_D      GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_1)
#define KEY_PIN_L      GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_2)
#define KEY_PIN_R      GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_3)
#define KEY_PIN_M      GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)
#define KEY_PIN_UD     GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_11)
#define KEY_PIN_C      GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_10)
#define KEY_PIN_DE     GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_9)
#define KEY_PIN_OK     GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_8)

void key_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//PB3ÓÃ×÷ÆÕÍ¨IO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 |
	                              GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
	                              GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIOC, &GPIO_InitStructure);	   
}




