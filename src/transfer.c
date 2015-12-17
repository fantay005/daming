#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "protocol.h"
#include "misc.h"
#include "norflash.h"
#include "transfer.h"
#include "libupdater.h"

#define BAUD                19200

#define  ComX               USART1

#define Com_IRQ             USART1_IRQn

#define  GPIO_Trans         GPIOA
#define  Trans_Tx           GPIO_Pin_9
#define  Trans_Rx           GPIO_Pin_10

static  xQueueHandle __TransQueue;

#define TRANS_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 2048)

void TransCmdSendChar(char c) {
	USART_SendData(ComX, c);
	while (USART_GetFlagStatus(ComX, USART_FLAG_TXE) == RESET);
}

static void TransCmdSendStr(char *s, int len){
	int i;
	
	for(i = 0; i < len; i++){
		TransCmdSendChar(*s++);
	}
}
	
typedef enum {
	TYPE_NONE = 0,
	TYPE_SEND,
	TYPE_RECIEV,
	TYPE_PROTOCOL_DATA,
} TransTaskMessageType;

typedef struct {
	TransTaskMessageType type;
	unsigned char length;
	char infor[200];
} TransTaskMessage;

static inline void *__TransGetMessageData(TransTaskMessage *message) {
	return message->infor;
}

bool TransTaskSendData(const char *dat, int len) {
	TransTaskMessage message;
	
	message.type = TYPE_SEND;
	message.length = len;
	memcpy(message.infor, dat, len);
	
	if (pdTRUE != xQueueSend(__TransQueue, &message, configTICK_RATE_HZ * 5)) {
		return false;
	}
	return true;
}

static void __TransInitUsart(int baud) {
	USART_InitTypeDef USART_InitStructure;
	
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(ComX, &USART_InitStructure);
	USART_ITConfig(ComX, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(ComX, ENABLE);
}

static void __TransInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  Trans_Tx;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_Trans, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = Trans_Rx;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_Trans, &GPIO_InitStructure);				  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitStructure.NVIC_IRQChannel = Com_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
}


static char buffer[200];
static unsigned char bufferIndex = 0;
static char isIPD = 0;
static unsigned char lenIPD = 0;


static inline void __gmsReceiveIPDData(unsigned char data) {
	char param1, param2;
	
	buffer[bufferIndex++] = data;
	if(bufferIndex == 15) {
		if (buffer[13] > '9') {
			param1 = buffer[13] - '7';
		} else {
			param1 = buffer[13] - '0';
		}
		
		if (buffer[14] > '9') {
			param2 = buffer[14] - '7';
		} else {
			param2 = buffer[14] - '0';
		}
		
		lenIPD = (param1 << 4) + param2;
	}

	if ((bufferIndex == (lenIPD + 18)) && (data == 0x03)){
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		TransTaskMessage message;
		buffer[bufferIndex++] = 0;

		message.type = TYPE_PROTOCOL_DATA;
		message.length = bufferIndex;
		memcpy(message.infor, buffer, bufferIndex);
		
		if (pdTRUE == xQueueSendFromISR(__TransQueue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isIPD = 0;
		bufferIndex = 0;
		lenIPD = 0;
		
	} else if (bufferIndex > (lenIPD + 18)) {
		isIPD = 0;
		bufferIndex = 0;
		lenIPD = 0;	
	}
}

void USART1_IRQHandler(void) {
	unsigned char data;
	if (USART_GetITStatus(ComX, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(ComX);
	
#if defined (__MODEL_DEBUG__)	
	USART_SendData(UART5, data);
#endif	
	USART_ClearITPendingBit(ComX, USART_IT_RXNE);
	
	if (isIPD) {
		__gmsReceiveIPDData(data);		
		return;
	}
	
	if (data == 0x0A) {
		buffer[bufferIndex++] = 0;
		if (bufferIndex >= 2) {
			portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
			TransTaskMessage message;
			
			message.type = TYPE_RECIEV;
			message.length = bufferIndex;
			memcpy(message.infor, buffer, bufferIndex);
			
			xQueueSendFromISR(__TransQueue, &message, &xHigherPriorityTaskWoken);

			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
		if(data == 0x02) {
			bufferIndex = 0;
			buffer[bufferIndex++] = data;
			isIPD = 1;
		}
	}
}

void __handleSendData(TransTaskMessage *p) {

	TransCmdSendStr(p->infor, p->length);
}

void __handleRecievData(TransTaskMessage *p) {
	
}

extern const char *GetBack(void);

void __handleProtocolDat(TransTaskMessage *p) {
	GMSParameter g;
	const char *dat = __TransGetMessageData(p);
	ProtocolHead *h = pvPortMalloc(sizeof(ProtocolHead));
	
	h->header = 0x02;
	sscanf((const char *)dat, "%*1s%10s", h->addr);
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	if((strncmp((char *)&(h->addr), (char *)GetBack(), 10) != 0) && (strncmp((char *)&(h->addr), (char *)&(g.GWAddr), 10) != 0)){
		return;
	}
	
	memcpy(&(h->addr), (const char *)&(g.GWAddr), 10);

	sscanf((const char *)dat, "%*11s%2s", h->contr);
	sscanf((const char *)dat, "%*13s%2s", h->lenth);

	__handleInternalProtocol(h, (char *)dat);

	vPortFree(h);
}

typedef struct {
	TransTaskMessageType type;
	void (*handlerFunc)(TransTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_SEND, __handleSendData},
	{ TYPE_RECIEV, __handleRecievData},
  { TYPE_PROTOCOL_DATA, __handleProtocolDat},
	{ TYPE_NONE, NULL },
};

static const unsigned int __RequiredFlag = 0xF8F88F8F;
static struct UpdateMark *const __mark = (struct UpdateMark *)(0x0800F800);
extern unsigned char *ProtocolMessage(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);
extern unsigned short GetLux(void);

static void __transTask(void *parameter) {
	portBASE_TYPE rc;
	TransTaskMessage message;
	unsigned char *buf;
	GMSParameter g;
	portTickType lastT;
	portTickType curT;	
	
	vTaskDelay(configTICK_RATE_HZ * 2);
	
	printf("start");
	if (__mark->RequiredFlag == __RequiredFlag) {
		char temp[10], len;
		
		printf("comein");
		sprintf(temp, "%02d%06d", __mark->type, __mark->SizeOfPAK);
		NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter)  + 1)/ 2);
		buf = ProtocolMessage(g.GWAddr, "37", temp, (unsigned char *)&len);
		TransCmdSendStr((char *)buf, len);
		vPortFree((void *)buf);	
	}
	
	printf("End");
	for (;;) {
//		printf("Trans : loop again\n");	
		curT = xTaskGetTickCount();		
		rc = xQueueReceive(__TransQueue, &message, configTICK_RATE_HZ / 10);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != TYPE_NONE; ++map) {
				if (message.type == map->type) {
					map->handlerFunc(&message);
					break;
				}
			}			
		} else if((curT - lastT) > configTICK_RATE_HZ * 5){
//			char ret[] = {0x02, 0x30, 0x35, 0x35, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x36, 0x32, 0x30, 0x31, 0x32, 0x30, 0x30, 0x30,
//										0x31, 0x31, 0x31, 0x36, 0x31, 0x2E, 0x31, 0x39, 0x30, 0x2E, 0x33, 0x38, 0x2E, 0x34, 0x36, 0x31, 0x43, 0x03};
//			TransCmdSendStr(ret, sizeof(ret));
			lastT = curT;
		}			
	}
}

void TransInit(void) {
	__TransInitHardware();
	__TransInitUsart(BAUD);
	__TransQueue = xQueueCreate(20, sizeof( TransTaskMessage));
	xTaskCreate(__transTask, (signed portCHAR *) "TRANS", TRANS_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

