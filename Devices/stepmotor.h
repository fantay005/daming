#ifndef __STEPMOTOR_H__
#define __STEPMOTOR_H__
#include "stm32f10x.h"


void StepMotor_Config(void);
void StepMotor_Forw(uint32_t step);
void StepMotor_Back(uint32_t step);
void StepMotor_Stop();

#endif

