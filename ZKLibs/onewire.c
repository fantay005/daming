
#include "onewire.h"

static
int __ow_start(One_Wire_t *OWx)
{
	int ret = 0;
	__disable_irq;
	OWx->set(true);
	OWx->set(false);
	OWx->delayUs(480);
	OWx->set(true);
	OWx->delayUs(70);
	ret = OWx->get();
	OWx->delayUs(480);
	__enable_irq;	
	return ret == 0;
}

static
void __ow_write_bit(One_Wire_t *OWx,int bit) {			
	OWx->set(false);
	if (bit) {
		OWx->delayUs(6);
		OWx->set(true);
		OWx->delayUs(64);
		return;
	}
	
	OWx->delayUs(60);
	OWx->set(true);
	OWx->delayUs(10);
	return;	
}


static int __ow_read_bit(One_Wire_t *OWx) {
		int ret;
		OWx->set(false);
		OWx->delayUs(6);
		OWx->set(true);
		OWx->delayUs(9);
		ret = OWx->get();
		OWx->delayUs(55);
		return ret;	
}

void OneWire_Reset(One_Wire_t *OWx)
{
	__ow_start(OWx);
}

void OneWire_Write(One_Wire_t *OWx,uint8_t dat)
{
	char i;
	__disable_irq;
	for(i = 0; i < 8; i++){			
		__ow_write_bit(OWx,dat & 0x01);
		dat = dat >> 1;
	}
	__enable_irq;
}

uint8_t OneWire_read(One_Wire_t *OWx)
{
	int i,dat = 0;
	__disable_irq;
	for(i = 0x01; i <= 0x80; i = i << 1){
		if (__ow_read_bit(OWx)) {
			dat |= i;
		}
	}
	__enable_irq;
	return dat;
}


