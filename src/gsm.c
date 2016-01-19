#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "rtc.h"
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "protocol.h"
#include "misc.h"
#include "zklib.h"
#include "norflash.h"
#include "second_datetime.h"
#include "elegath.h"

#define BROACAST   "9999999999"

#define GSM_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 1024 * 5)

#define RELAY_EXTI          EXTI15_10_IRQn
/// GSM task message queue.
static xQueueHandle __queue;

static GMSParameter __gsmRuntimeParameter;

void ATCmdSendChar(char c) {
	USART_SendData(USART3, c);
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

typedef enum {
	TYPE_NONE = 0,
	TYPE_SMS_DATA,
	TYPE_GPRS_DATA,
	TYPE_RTC_DATA,
	TYPE_SEND_TCP_DATA,
	TYPE_CSQ_DATA,
	TYPE_RESET,
	TYPE_SEND_AT,
	TYPE_SEND_SMS,
	TYPE_SETIP,
} GsmTaskMessageType;

typedef struct {
	GsmTaskMessageType type;
	unsigned char length;
	char infor[200];
} GsmTaskMessage;

static inline void *__gsmGetMessageData(GsmTaskMessage *message) {
	return message->infor;
}

bool GsmTaskSendAtCommand(const char *atcmd) {
	int len = strlen(atcmd);
	GsmTaskMessage message;
	
	message.type = TYPE_SEND_AT;
	message.length = len;
	memcpy(message.infor, atcmd, len);
	
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 5)) {
		return false;
	}
	return true;

}

bool GsmTaskSendTcpData(const char *dat, unsigned char len) {
	GsmTaskMessage message;
	
	message.type = TYPE_SEND_TCP_DATA;
	message.length = len;
	memcpy(message.infor, dat, len);	
	
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		return true;
	}
	return false;
}

static void __gsmInitUsart(int baud) {
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART3, ENABLE);
}

/// Init the CPU on chip hardware for the GSM modem.
static void __gsmInitHardware(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);				   //GSMÄ£¿éµÄ´®¿Ú

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

typedef struct {
	const char *prefix;
	GsmTaskMessageType type;
} GSMAutoReportMap;

static const GSMAutoReportMap __gsmAutoReportMaps[] = {
//	{ "+CMT", TYPE_SMS_DATA },
	{ NULL, TYPE_NONE },
};



static char buffer[200];
static char bufferIndex = 0;
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
		GsmTaskMessage message;
		buffer[bufferIndex++] = 0;
		
		message.type = TYPE_GPRS_DATA;
		message.length = bufferIndex;
		memcpy(message.infor, buffer, bufferIndex);	
		
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
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


void USART3_IRQHandler(void) {
	unsigned char data;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(USART3);
#if defined (__MODEL_DEBUG__)	
	USART_SendData(UART5, data);
#endif	
	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	if (isIPD) {
		__gmsReceiveIPDData(data);		
		return;
	}

	if (data == 0x0A) {
		buffer[bufferIndex++] = 0;
		if (bufferIndex >= 2) {
			portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
			const GSMAutoReportMap *p;
			for (p = __gsmAutoReportMaps; p->prefix != NULL; ++p) {
				if (strncmp(p->prefix, buffer, strlen(p->prefix)) == 0) {
					GsmTaskMessage message;
					
					message.type = p->type;
					message.length = bufferIndex;
					memcpy(message.infor, buffer, bufferIndex);	
					
					xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken);
					if (xHigherPriorityTaskWoken)
						taskYIELD();
				}
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

const char *GetBack(void){
	const char *data = BROACAST;
	return data;
}

void __handleProtocol(GsmTaskMessage *msg) {
	GMSParameter g;
	ProtocolHead *h = pvPortMalloc(sizeof(ProtocolHead));

	
	h->header = 0x02;
	sscanf((const char *)msg->infor, "%*1s%10s", h->addr);
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	if((strncmp((char *)&(h->addr), (char *)GetBack(), 10) != 0) && (strncmp((char *)&(h->addr), (char *)&(g.GWAddr), 10) != 0)){
		
		memcpy(g.GWAddr, h->addr, 10);
		NorFlashWrite(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
		return;
	}

	sscanf((const char *)msg->infor, "%*11s%2s", h->contr);
	sscanf((const char *)msg->infor, "%*13s%2s", h->lenth);

	ProtocolHandler(h, msg->infor);

	vPortFree(h);
}

void __handleSendTcpDataLowLevel(GsmTaskMessage *msg) {
	int i, len;
	char *ret;
	
	ret = msg->infor;
	len = msg->length; 
#if defined (__MODEL_DEBUG__)	
	 printf("%s.\r\n", __gsmGetMessageData(msg));
#endif	
	for(i = 0; i < len; i++)
		ATCmdSendChar(*ret++);
}

void trans(char *tmpa, char tmpb, char *tmpd){
		if((tmpd[tmpb-3] - '0') > 0){
			*tmpa = (tmpd[tmpb - 3] - '0') * 10 + (tmpd[tmpb - 2] - '0');
		} else {
			*tmpa = (tmpd[tmpb - 2] - '0');
		}
}

void __handleM35RTC(GsmTaskMessage *msg) {
	DateTime dateTime;
	
	RtcSetTime(DateTimeToSecond(&dateTime));
}


typedef struct {
	GsmTaskMessageType type;
	void (*handlerFunc)(GsmTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {

	{ TYPE_GPRS_DATA, __handleProtocol },
	{ TYPE_SEND_TCP_DATA, __handleSendTcpDataLowLevel },
	{ TYPE_NONE, NULL },
};

static void __gsmTask(void *parameter) {
	portBASE_TYPE rc;
	GsmTaskMessage message;
	portTickType lastT = 0;
	portTickType curT;	

	for (;;) {
//		printf("Gsm: loop again\n");					
		curT = xTaskGetTickCount();
		rc = xQueueReceive(__queue, &message, portMAX_DELAY);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != TYPE_NONE; ++map) {
				if (message.type == map->type) {
					map->handlerFunc(&message);
					break;
				}
			}
		} else if((curT - lastT) > configTICK_RATE_HZ * 3){
			
			NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&__gsmRuntimeParameter, (sizeof(GMSParameter)  + 1)/ 2);
			
			if(__gsmRuntimeParameter.GWAddr[0] == 0xFF){
				continue;
			}
			
			lastT = curT;
		}
	}
}
 
void GSMInit(void) {
	__gsmInitHardware();
	__gsmInitUsart(19200);
	__queue = xQueueCreate(30, sizeof( GsmTaskMessage));
	xTaskCreate(__gsmTask, (signed portCHAR *) "GSM", GSM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}
