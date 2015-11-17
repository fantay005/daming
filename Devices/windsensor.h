#ifndef __WINSENSOR_H__
#define __WINSENSOR_H__

#include "stm32f10x.h"

void WindSensor_Config(void);
uint32_t WindSpeed_Read(void);
uint32_t WindDir_Read(void);

#endif

