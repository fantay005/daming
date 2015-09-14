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
#include "shuncom.h"

#define COMx         USART2
#define COMx_IRQn    USART2_IRQn

#define Pin_COMx_TX  GPIO_Pin_2
#define Pin_COMx_RX  GPIO_Pin_3
#define GPIO_COMx    GPIOA

#define COMX_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 256)

static xQueueHandle __comxQueue;

static void __CommInitUsart(int baud) {
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(COMx, &USART_InitStructure);
	USART_ITConfig(COMx, USART_IT_RXNE, ENABLE);
	USART_Cmd(COMx, ENABLE);
}

static void __CommInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  Pin_COMx_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_COMx, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = Pin_COMx_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_COMx, &GPIO_InitStructure);				   //ZigBee模块的串口

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				    //ZigBee模块的配置脚

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = COMx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

typedef enum{
	TYPE_RECIEVE_DATA,
	TYPE_SEND_DATA,
	TYPE_MODIFY_DATA,
	TYPE_NONE,
}ComxTaskMessageType;

typedef struct {
	/// Message type.
	ComxTaskMessageType type;
	/// Message lenght.
	unsigned char length;
} ComxTaskMsg;

static ComxTaskMsg *__ComxCreateMessage(ComxTaskMessageType type, const char *dat, unsigned char len) {
  ComxTaskMsg *message = pvPortMalloc(ALIGNED_SIZEOF(ComxTaskMsg) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

static inline void *__ComxGetMsgData(ComxTaskMsg *message) {
	return &message[1];
}

void __ComxDestroyMessage(ComxTaskMsg *message) {
	vPortFree(message);
}

static void ComxComSendChar(char c) {
	USART_SendData(COMx, c);
	while (USART_GetFlagStatus(COMx, USART_FLAG_TXE) == RESET);
}

bool ComxTaskRecieveModifyData(const char *dat, int len) {
	ComxTaskMsg *message = __ComxCreateMessage(TYPE_MODIFY_DATA, dat, len);
	if (pdTRUE != xQueueSend(__comxQueue, &message, configTICK_RATE_HZ * 5)) {
		__ComxDestroyMessage(message);
		return true;
	}
	return false;
}

static unsigned char bufferIndex;
static unsigned char buffer[255];
static unsigned char LenZIGB;

void USART2_IRQHandler(void) {
	unsigned char data;
	char param1, param2;
	
	if (USART_GetITStatus(COMx, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(COMx);
//	USART_SendData(USART1, data);
	USART_ClearITPendingBit(COMx, USART_IT_RXNE);

	if(((data >= '0') && (data <= 'F')) || (data == 0x03)){
		buffer[bufferIndex++] = data;
		if(bufferIndex == 9) {
			if (buffer[7] > '9') {
				param1 = buffer[7] - '7';
			} else {
				param1 = buffer[7] - '0';
			}
			
			if (buffer[8] > '9') {
				param2 = buffer[8] - '7';
			} else {
				param2 = buffer[8] - '0';
			}
			
			LenZIGB = (param1 << 4) + param2;
		}
	} else {
		bufferIndex = 0;
		LenZIGB = 0;
	}
	
	if ((bufferIndex == (LenZIGB + 12)) && (data == 0x03)){
		ComxTaskMsg *msg;
		portBASE_TYPE xHigherPriorityTaskWoken;
		buffer[bufferIndex++] = 0;
		msg = __ComxCreateMessage(TYPE_RECIEVE_DATA, (const char *)buffer, bufferIndex);		
		if (pdTRUE == xQueueSendFromISR(__comxQueue, &msg, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				portYIELD();
			}
		}
		bufferIndex = 0;
		LenZIGB = 0;
	} else if(bufferIndex > (LenZIGB + 12)){
		bufferIndex = 0;
		LenZIGB = 0;
	} else if (data == 0x02){
		bufferIndex = 0;
		buffer[bufferIndex++] = data;
	}
}

static void __handleRecieve(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
	
}

static void __handleSend(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
	
}

static void __handleModify(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
	
	
}

typedef struct {
	ComxTaskMessageType type;
	void (*handlerFunc)(ComxTaskMsg *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_RECIEVE_DATA, __handleRecieve },
	{ TYPE_SEND_DATA, __handleSend },
	{ TYPE_MODIFY_DATA, __handleModify}, 
	{ TYPE_NONE, NULL },
};

static void __comxTask(void *parameter) {
	portBASE_TYPE rc;
	ComxTaskMsg *message;

	for (;;) {
		printf("COMM: loop again\n");
		rc = xQueueReceive(__comxQueue, &message, configTICK_RATE_HZ * 10);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != TYPE_NONE; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			__ComxDestroyMessage(message);
		} 
	}
}


void CommInit(void) {
	__CommInitUsart(9600);
	__CommInitHardware();
	__comxQueue = xQueueCreate(5, sizeof(ComxTaskMsg *));
	xTaskCreate(__comxTask, (signed portCHAR *) "COM", COMX_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

