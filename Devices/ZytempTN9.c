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

#define GPIOx_Tirg	GPIOA
#define PINx_Tirg 	GPIO_Pin_7

#if defined(STM32_M3)
#define SPIx		SPI2
#define SPIx_Irq	SPI2_IRQn
#define SPIx_Rcc	RCC_APB1Periph_SPI2
#define GPIOx_SPIx	GPIOB
#define PINx_Sck	GPIO_Pin_13
#define PINx_Miso	GPIO_Pin_14
#else
#define SPIx		SPI1
#define SPIx_Irq	SPI1_IRQn
#define SPIx_Rcc	RCC_APB2Periph_SPI1
#define GPIOx_SPIx	GPIOA
#define PINx_Sck	GPIO_Pin_5
#define PINx_Miso	GPIO_Pin_6
#endif

#define Tirg_Set()		GPIO_SetBits(GPIOx_Tirg,PINx_Tirg)
#define Tirg_Reset()	GPIO_ResetBits(GPIOx_Tirg,PINx_Tirg)
#define __SendData(type,dat)		Dev_SendIntWithType(DEV_ZYTEMP,(dat),(type), IN_ISR)

/*
 * spi从模式单工通信(SPI_CR1[BIDIMODE] 开启该模式)
 * PB13	SCK作为时钟
 * PB14	MISO作为数据(SPI_CR2[BIDIOE]控制数据方向)
 */
static
void __SPI_Init(void)
{
	/* 配置GPIO */{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = PINx_Sck | PINx_Miso;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx_SPIx, &GPIO_InitStructure);
	}
	
	/*配置SPI中断*/{
    NVIC_InitTypeDef NVIC_InitStructure; 
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  													
    NVIC_InitStructure.NVIC_IRQChannel = SPIx_Irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	}

	/* 配置SPI */{
	SPI_InitTypeDef  SPI_InitStructure;
#if defined(STM32_M3)	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
#else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
#endif
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Rx;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;		//从模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;	//数据8bit
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;			//空闲时钟为高电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;		//时钟第一个边沿采数据
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//高位先传
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPIx, &SPI_InitStructure);
	}
	SPI_I2S_ITConfig(SPIx, SPI_I2S_IT_RXNE, ENABLE);
}

static
void __set_direction(uint16_t SPI_Direction)
{
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Pin = PINx_Tirg;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	if(SPI_Direction == SPI_Direction_Rx){
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	}else if(SPI_Direction == SPI_Direction_Tx){
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	}else return;
	GPIO_Init(GPIOx_Tirg, &GPIO_InitStructure);
	SPI_BiDirectionalLineConfig(SPIx, SPI_Direction);
}

static
void __complete_measure_tamb(int temp)
{
	__SendData("tamb",temp);
}

static
void __complete_measure_tobj(int temp)
{
	__SendData("tobj",temp);
}
static
void __check_data(uint8_t dat)
{
	static uint8_t buf[2*sizeof(TN9data_t)];
	static int8_t index = 0;
	uint16_t data;	
	TN9data_t *ptr;
//	printf("%02X ",dat);
	buf[index++] = dat;
	if(dat == '\r'){
		if(index >= sizeof(TN9data_t)){
			ptr = (TN9data_t *)&buf[index - sizeof(TN9data_t)];
			if(ptr->sum == (uint8_t)(ptr->item+ptr->msb+ptr->lsb)){
				data = (ptr->msb << 8)|(ptr->lsb);
				switch(ptr->item){
				case TN9_TOBJ:
					__complete_measure_tobj(data);
					break;
				case TN9_TAMB:
					__complete_measure_tamb(data);
					break;
				}
				index = 0;
			}
		}
	}else if(index >= sizeof(buf)){
		index = 0;
	}	
}

void ZytempTN9_Start(void)
{
	SPI_Cmd(SPIx, ENABLE);
	Tirg_Reset();
}
void ZytempTN9_Stop(void)
{
	SPI_Cmd(SPIx, DISABLE);
	Tirg_Set();
}
 
void ZytempTN9_Config(void)
{
	__set_direction(SPI_Direction_Rx);
	__SPI_Init();
}

void ZytempTN9_SetRatio(uint8_t ratio)
{
	int ret;
	int tirg = GPIO_ReadOutputDataBit(GPIOx_Tirg, PINx_Tirg);
	ZytempTN9_Stop();
	ret = ZytempTN9_SetEmissivity(ratio);
	ZytempTN9_Config();
	if(tirg == 0){
		ZytempTN9_Start();
	}
}


#if defined(STM32_M3)
void SPI2_IRQHandler(void)
#else
void SPI1_IRQHandler(void)
#endif
{
	uint16_t dat;
	if(SPI_I2S_GetITStatus(SPIx, SPI_I2S_IT_RXNE) != RESET){
		SPI_I2S_ClearITPendingBit(SPIx, SPI_I2S_IT_RXNE);
		dat = SPI_I2S_ReceiveData(SPIx);
		__check_data(dat);
	}
}

