#ifndef __WEIGHINGSENSOR_H__
#define __WEIGHINGSENSOR_H__
#include "stm32f10x.h"
#include "devices.h"

typedef enum {
	HX711_A_128,
	HX711_B_32,
	HX711_A_64
}HX711_Mode_Sel;


void WeighingSensor_Config(void);
void WeighingSensor_Start();
void WeighingSensor_Stop();

#endif
