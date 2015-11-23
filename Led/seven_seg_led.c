#include <string.h>
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "seven_seg_led.h"

static unsigned short __displayChar[SEVEN_SEG_LED_NUM] = {0x8000, 0x4000, 0x2000, 0x1000};
static char __changed;
static xSemaphoreHandle __semaphore = NULL;

#define COMMON_IS_ANODE 1 // 1-共阳; 0-共阴;

#define CHANNEL0_DATA_GPIO_PORT  GPIOC
#define CHANNEL0_DATA_GPIO_PIN   GPIO_Pin_13         //数据输入
#define CLK245_GPIO_PORT GPIOC
#define CLK245_GPIO_PIN  GPIO_Pin_15                 //移位寄存器时钟
#define LAUNCH_GPIO_PORT  GPIOC
#define LAUNCH_GPIO_PIN   GPIO_Pin_14                //输出寄存器时钟

static inline void  __channel0SetDataHigh(void) {
	GPIO_SetBits(CHANNEL0_DATA_GPIO_PORT, CHANNEL0_DATA_GPIO_PIN);
}

static inline void __channel0SetDataLow(void) {
	GPIO_ResetBits(CHANNEL0_DATA_GPIO_PORT, CHANNEL0_DATA_GPIO_PIN);
}

static inline void __setClkHigh(void) {
	GPIO_SetBits(CLK245_GPIO_PORT, CLK245_GPIO_PIN);
}

static inline void __setClkLow(void) {
	GPIO_ResetBits(CLK245_GPIO_PORT, CLK245_GPIO_PIN);
}

static inline void __setLaunchHigh(void) {
	GPIO_SetBits(LAUNCH_GPIO_PORT, LAUNCH_GPIO_PIN);
}

static inline void __setLaunchLow(void) {
	GPIO_ResetBits(LAUNCH_GPIO_PORT, LAUNCH_GPIO_PIN);
}

static void __shiftByte(unsigned short c0) {
	unsigned int bit;
	for (bit = 0x01; bit != 0x10000; bit = bit << 1) {
		__setClkLow();
		if (bit & c0) {
			__channel0SetDataHigh();
		} else {
			__channel0SetDataLow();
		}
		__setClkHigh();
	}
}

void inline __display(void) {
	if (__changed) {
		int i;
		for (i = 0; i < SEVEN_SEG_LED_NUM; ++i) {
			__setLaunchLow();
			__shiftByte(__displayChar[i]);
			__setLaunchHigh();
		}
	}
}


void SevenSegLedInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	if (__semaphore != NULL) {
		return;
	}

	__changed = 1;

	vSemaphoreCreateBinary(__semaphore);

	__channel0SetDataHigh();
	__setClkHigh();
	__setLaunchHigh();

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	GPIO_InitStructure.GPIO_Pin = CHANNEL0_DATA_GPIO_PIN;
	GPIO_Init(CHANNEL0_DATA_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = CLK245_GPIO_PIN;
	GPIO_Init(CLK245_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LAUNCH_GPIO_PIN;
	GPIO_Init(LAUNCH_GPIO_PORT, &GPIO_InitStructure);

	__display();
}

static char __charToDisplayContent(uint8_t c) {
#if COMMON_IS_ANODE
	
	static const uint8_t displayTable[] = {0x03, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x01, 0x09,
                                         0x10, 0xC0, 0xE4, 0x84, 0x60, 0x70};   //    0~9
#else

	static const uint8_t displayTable[] = {
		(uint8_t)~0x03, (uint8_t)~0x9F, (uint8_t)~0x25, (uint8_t)~0x0D,
		(uint8_t)~0x99, (uint8_t)~0x49, (uint8_t)~0x41, (uint8_t)~0x1F,
		(uint8_t)~0x01, (uint8_t)~0x09, (uint8_t)~0x11, (uint8_t)~0xC1,
		(uint8_t)~0xE4, (uint8_t)~0x85, (uint8_t)~0x61, (uint8_t)~0x71
	};
#endif
	if (c >= sizeof(displayTable)) {
		return 0;
	}
	return displayTable[c];
}

bool SevenSegLedSetContent(uint16_t what) {
	unsigned short orig = 0x8000;
	char i, j;
	
	for(i = 0; i < 4; i++){
		j = (3 - i) * 4;
		xSemaphoreTake(__semaphore, portMAX_DELAY);
		__displayChar[i] = __charToDisplayContent((what >> j) & 0xF) | orig;	
		xSemaphoreGive(__semaphore);
		
		orig /= 2;	
	}
	
	__display();
	return 1;
}


void SevenSegLedDisplay(void) {
	xSemaphoreTake(__semaphore, portMAX_DELAY);
	__display();
	xSemaphoreGive(__semaphore);
}
