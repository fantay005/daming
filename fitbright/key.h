#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x_gpio.h"

#define  KEY_IO_0     GPIOB     
#define  KEY_Pin_0    GPIO_Pin_8 

#define  KEY_IO_1     GPIOB     
#define  KEY_Pin_1    GPIO_Pin_0 

#define  KEY_IO_2     GPIOC     
#define  KEY_Pin_2    GPIO_Pin_1  

#define  KEY_IO_3     GPIOC   
#define  KEY_Pin_3    GPIO_Pin_10 

#define  KEY_IO_4     GPIOB    
#define  KEY_Pin_4    GPIO_Pin_1

#define  KEY_IO_5     GPIOB   
#define  KEY_Pin_5    GPIO_Pin_2  

#define  KEY_IO_6     GPIOB  
#define  KEY_Pin_6    GPIO_Pin_3

#define  KEY_IO_7     GPIOB     
#define  KEY_Pin_7    GPIO_Pin_4 

#define  KEY_IO_8     GPIOB   
#define  KEY_Pin_8    GPIO_Pin_5   

#define  KEY_IO_9     GPIOB   
#define  KEY_Pin_9    GPIO_Pin_6 

#define  KEY_IO_Lf    GPIOC  
#define  KEY_Pin_Lf   GPIO_Pin_2 

#define  KEY_IO_Rt    GPIOC   
#define  KEY_Pin_Rt   GPIO_Pin_3 

#define  KEY_IO_Up    GPIOA  
#define  KEY_Pin_Up   GPIO_Pin_5 

#define  KEY_IO_Dn    GPIOC  
#define  KEY_Pin_Dn   GPIO_Pin_0 

void key_init(void);
void KEY(void);

#endif /* __KEY_H */
