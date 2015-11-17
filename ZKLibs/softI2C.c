#include "softi2c.h"

static
void __send_bit(const SoftI2C_t *I2Cx,bool bit)
{
	unsigned char reTry = 10;
	I2Cx->setSDA(bit);
	I2Cx->delayUs(I2Cx->speed);
	I2Cx->setSCL(1);	
	while(!I2Cx->getSCL()){
		I2Cx->delayUs(I2Cx->speed);	
		if(reTry-- == 0) break; 
	}
	I2Cx->delayUs(I2Cx->speed);
	I2Cx->setSCL(0);
}

static
bool __recv_bit(const SoftI2C_t *I2Cx)
{
	unsigned char reTry = 10;
	bool ret;
	I2Cx->delayUs(I2Cx->speed);	
	I2Cx->setSCL(1);
	while(!I2Cx->getSCL()){
		I2Cx->delayUs(I2Cx->speed);	
		if(reTry-- == 0) break;
	}
	ret = I2Cx->getSDA();
	I2Cx->delayUs(I2Cx->speed);	
	I2Cx->setSCL(0);
	return ret;
}

void SoftI2C_Start(const SoftI2C_t *I2Cx)
{
	I2Cx->setSDA(1);
	I2Cx->setSCL(1);
	I2Cx->delayUs(I2Cx->speed);
	I2Cx->setSDA(0);
	I2Cx->delayUs(I2Cx->speed);
	I2Cx->setSCL(0);
	I2Cx->delayUs(I2Cx->speed);
}

void SoftI2C_Stop(const SoftI2C_t *I2Cx)
{
	I2Cx->setSDA(0);
	I2Cx->delayUs(I2Cx->speed);
	I2Cx->setSCL(1);
	I2Cx->delayUs(I2Cx->speed);
	I2Cx->setSDA(1);
	I2Cx->delayUs(I2Cx->speed);
}

unsigned char SoftI2C_Read(const SoftI2C_t *I2Cx, bool ack)
{
	unsigned char ret = 0;
	unsigned char bit;
	
	I2Cx->setSDA(1);
	for (bit = 0x80; bit != 0; bit = bit >> 1) {
		if (__recv_bit(I2Cx)) {
			ret |= bit;
		}
	}
	if (ack) {
		__send_bit(I2Cx,0);
	}
	
	return ret;
}

bool SoftI2C_Write(const SoftI2C_t *I2Cx, unsigned char dat)
{
	unsigned char bit;
	for (bit = 0x80; bit != 0; bit = bit >> 1) {
		__send_bit(I2Cx, bit & dat);
	}

	I2Cx->setSDA(1);
	return !__recv_bit(I2Cx);
}


