#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include "zklib.h"
#include "CommZigBee.h"
#include "ili9320.h"
#include "display.h"

#define SERx         USART3
#define SERx_IRQn    USART3_IRQn

#define Pin_SERx_TX  GPIO_Pin_10
#define Pin_SERx_RX  GPIO_Pin_11
#define GPIO_SERx    GPIOB

#define CONFIG_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 256)

static xQueueHandle __ConfigQueue;

static void __ConfigInitUsart(int baud) {
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(SERx, &USART_InitStructure);
	USART_ITConfig(SERx, USART_IT_RXNE, ENABLE);
	USART_Cmd(SERx, ENABLE);
}

static void __ConfigInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  Pin_SERx_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_SERx, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = Pin_SERx_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_SERx, &GPIO_InitStructure);				   //ZigBee模块的串口

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				    //ZigBee模块的配置脚

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = SERx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

typedef enum{
	CONFIG_RECIEVE_DATA,
	CONFIG_SEND_DATA,
	CONFIG_MODIFY_DATA,
	CONFIG_USELESS_DATA,
}ConfigTaskMessageType;

typedef struct {
	/// Message type.
	ConfigTaskMessageType type;
	/// Message lenght.
	unsigned char length;
} ConfigTaskMsg;

static ConfigTaskMsg *__ConfigCreateMessage(ConfigTaskMessageType type, const char *dat, unsigned char len) {
  ConfigTaskMsg *message = pvPortMalloc(ALIGNED_SIZEOF(ConfigTaskMsg) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

static inline void *__ConfigGetMsgData(ConfigTaskMsg *message) {
	return &message[1];
}

void __ConfigDestroyMessage(ConfigTaskMsg *message) {
	vPortFree(message);
}

static void ConfigComSendChar(char c) {
	USART_SendData(SERx, c);
	while (USART_GetFlagStatus(SERx, USART_FLAG_TXE) == RESET);
}

bool ConfigTaskRecieveModifyData(const char *dat, int len) {
	ConfigTaskMsg *message = __ConfigCreateMessage(CONFIG_MODIFY_DATA, dat, len);
	if (pdTRUE != xQueueSend(__ConfigQueue, &message, configTICK_RATE_HZ * 5)) {
		__ConfigDestroyMessage(message);
		return true;
	}
	return false;
}

static unsigned char bufferIndex;
static unsigned char buffer[255];

void USART3_IRQHandler(void) {
	unsigned char data;
	char param1, param2;
	
	if (USART_GetITStatus(SERx, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(SERx);
//	USART_SendData(USART1, data);
	USART_ClearITPendingBit(SERx, USART_IT_RXNE);

	if (data == 0x0A){
		ConfigTaskMsg *msg;
		portBASE_TYPE xHigherPriorityTaskWoken;
		buffer[bufferIndex++] = 0;
		msg = __ConfigCreateMessage(CONFIG_RECIEVE_DATA, (const char *)buffer, bufferIndex);		
		if (pdTRUE == xQueueSendFromISR(__ConfigQueue, &msg, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				portYIELD();
			}
		}
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static char Line = 1;

static void __TaskHandleRecieve(ConfigTaskMsg *msg){
	char *p = __ConfigGetMsgData(msg);
	if(Line == 1){
		BackColorSet();
	}
	Lcd_LineDisplay16(Line, (const unsigned char *)p);
	if(strlen(p) > 4){
		Line++;
	}
	
//	if(Line >=3){
//		Lcd_LineDisplay16(Line++, "大明科技");
//	}
}

static void __TaskHandleSend(ConfigTaskMsg *msg){
	char *p = __ConfigGetMsgData(msg);
	
}

static void __TaskHandleModify(ConfigTaskMsg *msg){
	char *p = __ConfigGetMsgData(msg);
	
}
typedef struct {
	ConfigTaskMessageType type;
	void (*handlerFunc)(ConfigTaskMsg *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ CONFIG_RECIEVE_DATA, __TaskHandleRecieve },
	{ CONFIG_SEND_DATA, __TaskHandleSend },
	{ CONFIG_MODIFY_DATA, __TaskHandleModify},
	{ CONFIG_USELESS_DATA, NULL },
};

static void __ConfigTask(void *parameter) {
	portBASE_TYPE rc;
	ConfigTaskMsg *message;

	for (;;) {
		printf("Config: loop again\n");
		rc = xQueueReceive(__ConfigQueue, &message, configTICK_RATE_HZ * 10);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != CONFIG_USELESS_DATA; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			__ConfigDestroyMessage(message);
		} 
	}
}


void ConfigInit(void) {
	__ConfigInitHardware();
	__ConfigInitUsart(38400);
	__ConfigQueue = xQueueCreate(5, sizeof(ConfigTaskMsg *));
	xTaskCreate(__ConfigTask, (signed portCHAR *) "CONFIG", CONFIG_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

