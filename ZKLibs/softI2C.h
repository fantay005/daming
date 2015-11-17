#ifndef __SOFT_I2C_H__
#define __SOFT_I2C_H__

#include <stdbool.h>

typedef struct {
	void (*setSCL)(bool isHigh);
	bool (*getSCL)(void);
	void (*setSDA)(bool isHigh);
	bool (*getSDA)(void);
	void (*delayUs)(int us);
	int speed;
} SoftI2C_t;

void SoftI2C_Start(const SoftI2C_t *I2Cx);
void SoftI2C_Stop(const SoftI2C_t *I2Cx);
unsigned char SoftI2C_Read(const SoftI2C_t *I2Cx, bool ack);
bool SoftI2C_Write(const SoftI2C_t *I2Cx, unsigned char dat);

#endif

