#ifndef __ONE_WIRE_H__
#define __ONE_WIRE_H__

#include "stm32f10x.h"
#include "stdbool.h"

typedef struct{
	void (*set)(bool b);
	uint8_t (*get)(void);
	void (*delayUs)(uint16_t n);
}One_Wire_t;

void OneWire_Reset(One_Wire_t *ow);
void OneWire_Write(One_Wire_t *ow,uint8_t dat);
uint8_t OneWire_read(One_Wire_t *ow);

#endif

