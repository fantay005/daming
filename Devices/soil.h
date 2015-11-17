#ifndef __SOIL_H__
#define __SOIL_H__

#include "stm32f10x.h"

void SoilSensor_Config(void);
uint32_t SoilTemp_Read(uint32_t index);
uint32_t SoilHum_Read(uint32_t index);

#endif

