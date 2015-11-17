#ifndef __MLX9061x_H__
#define __MLX9061x_H__

#include "softi2c.h"
#include "stm32f10x.h"

void MLX90614_Start(void);
void MLX90614_Stop(void);
void MLX90614_Config(void);

void MLX90615_Start(void);
void MLX90615_Stop(void);
void MLX90615_Config(void);

void MLX9061x_Start(void);
void MLX9061x_Stop(void);
void MLX9061x_Config(void);

uint16_t MLX9061x_memRead(const SoftI2C_t *i2c, uint8_t sAddr, uint8_t mAddr);

#endif

