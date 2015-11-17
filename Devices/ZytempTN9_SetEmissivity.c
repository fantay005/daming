#include "zytemptn9.h"
#include "misc.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_rcc.h"

typedef enum{
	TN9_TOBJ = 0x4c,	//'L'
	TN9_TAMB = 0x66,	//'f'
	TN9_RATIO = 0x53,	//'S'
}TN9_Item_ID;

typedef struct{
	uint8_t item;
	uint8_t msb;
	uint8_t lsb;
	uint8_t sum;
	uint8_t CR;
}TN9data_t;

typedef union{
	uint32_t v;
	struct{
		uint8_t reserver;
		uint8_t item;
		uint16_t val;
	}dat;
}TN9ret_t;

#define GPIOx_Tirg	GPIOA
#define PINx_Tirg 	GPIO_Pin_7

#if defined(STM32_M3)
#define GPIOx_SPIx	GPIOB
#define PINx_Sck	GPIO_Pin_13
#define PINx_Miso	GPIO_Pin_14
#else
#define GPIOx_SPIx	GPIOA
#define PINx_Sck	GPIO_Pin_5
#define PINx_Miso	GPIO_Pin_6
#endif

#define Tirg_Set()		GPIO_SetBits(GPIOx_Tirg,PINx_Tirg)
#define Tirg_Reset()	GPIO_ResetBits(GPIOx_Tirg,PINx_Tirg)
#define Sck_Get()		GPIO_ReadInputDataBit(GPIOx_SPIx, PINx_Sck)
#define Miso_Get()		GPIO_ReadInputDataBit(GPIOx_SPIx, PINx_Miso)

#define Sck_Set()		GPIO_SetBits(GPIOx_SPIx,PINx_Sck)
#define Sck_Reset()		GPIO_ResetBits(GPIOx_SPIx,PINx_Sck)
#define Miso_Set()		GPIO_SetBits(GPIOx_SPIx,PINx_Miso)
#define Miso_Reset()	GPIO_ResetBits(GPIOx_SPIx,PINx_Miso)

static
void __delayms(int ms)
{
	do{
		int i;
		for(i = 0; i < 6923; i++);
	}while(ms-- > 0);
}

static
void __delay50us(int us)
{
	do{
		int i;
		for(i = 0; i < 346; i++);
	}while(us-- > 0);
}

static 
void __gpio_init_rx(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = PINx_Tirg;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOx_Tirg, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = PINx_Sck;
	GPIO_Init(GPIOx_SPIx, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = PINx_Miso;
	GPIO_Init(GPIOx_SPIx, &GPIO_InitStructure);
	Tirg_Set();
}

static
void __gpio_init_tx(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	uint16_t dat = GPIO_ReadOutputData(GPIOx_SPIx);
	dat |= (PINx_Miso|PINx_Sck);
	GPIO_Write(GPIOx_SPIx,dat);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = PINx_Tirg;
	GPIO_Init(GPIOx_Tirg, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = PINx_Sck;
	GPIO_Init(GPIOx_SPIx, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PINx_Miso;
	GPIO_Init(GPIOx_SPIx, &GPIO_InitStructure);
}

static
uint8_t __Read8Bit(void)
{
	uint8_t tmp = 0;
	int i;
	for(i = 0; i < 8; i++){
		tmp <<= 1;
		while(Sck_Get());
		if(Miso_Get()) tmp |= 1;
		while(Sck_Get() == 0);
	}
	return tmp;
}

static
uint8_t __Read8Bit2(void)
{
	uint8_t tmp = 0;
	int i;
	for(i = 0; i < 8; i++){
		tmp <<= 1;
		while(Sck_Get());
		if(Miso_Get()) tmp |= 1;
		while(Sck_Get() == 0);
	}
	return tmp;
}

static
void __Write8Bit(uint8_t dat)
{
	int i;
	for(i = 7; i >= 0; i--){
		if((dat&(1<<i)) != 0){
			Miso_Set();
		}else {
			Miso_Reset();
		}
		__delay50us(5);
		Sck_Reset();__delay50us(5);
		Sck_Set();
	}
}



static
int __check_data1(uint8_t dat)
{
	static uint8_t buf[2*sizeof(TN9data_t)];
	static int8_t index = 0;
	TN9ret_t ret;
	TN9data_t *ptr;
	buf[index++] = dat;
	if(dat == '\r'){
		if(index >= sizeof(TN9data_t)){
			ptr = (TN9data_t *)(&buf[index - sizeof(TN9data_t)]);
			if(ptr->sum == (uint8_t)(ptr->item+ptr->msb+ptr->lsb)){
				index = 0;
				ret.dat.item = ptr->item;
				ret.dat.val = (ptr->msb<<8)|(ptr->lsb);
				return ret.v;
			}
		}
	}else if(index >= sizeof(buf)){
		index = 0;
		return -1;
	}
	return -1;
}


uint8_t buf[2*sizeof(TN9data_t)];
static int8_t index = 0;
static uint32_t temp = 0;

static
int __check_data2(uint8_t dat)
{
	TN9ret_t ret;
	TN9data_t *ptr;
	buf[index++] = dat;
	if(dat == '\r'){
		if(index >= sizeof(TN9data_t)){
			ptr = (TN9data_t *)(&buf[index - sizeof(TN9data_t)]);
			if(ptr->sum == (uint8_t)(ptr->item+ptr->msb+ptr->lsb)){
				index = 0;
				ret.dat.item = ptr->item;
				ret.dat.val = (ptr->msb<<8)|(ptr->lsb);
				return ret.v;
			}
		}
	}else if(index >= sizeof(buf)){
		index = 0;
		return -1;
	}
	return -1;
}

static
int __send_data(uint8_t item, uint8_t msb, uint8_t lsb)
{
	int i;
	TN9data_t tn9;
	char *ptr = (char *)&tn9;
	tn9.item = item;
	tn9.msb = msb;
	tn9.lsb = lsb;
	tn9.sum = (uint8_t )(tn9.item+tn9.msb+tn9.lsb);
	tn9.CR = '\r';
	for(i = 0; i < sizeof(TN9data_t); i++){
		__Write8Bit(ptr[i]);
	}
}
 
uint8_t ZytempTN9_SetEmissivity(uint8_t ratio)
{
	uint8_t dat;
	int ret;
	int k = 100;
	__gpio_init_rx();
	do{
		dat = __Read8Bit();
		ret = __check_data1(dat);
	}while(ret < 0);
	__disable_irq;
	__delayms(6);
	__gpio_init_tx();
	__send_data(TN9_RATIO, ratio, 0x04);
	__gpio_init_rx();
	__enable_irq;
	do{
		TN9ret_t *pRet;
		dat = __Read8Bit2();
		ret = __check_data2(dat);
		pRet = (TN9ret_t *)&dat;
		if(pRet->dat.item == TN9_RATIO){
			return ret;
		}
	}while(k-- > 0);
	return 0;
}

