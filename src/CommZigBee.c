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
#include "sdcard.h"
#include "key.h"
#include "ili9320.h"
#include "norflash.h"

#define COMx         USART2
#define COMx_IRQn    USART2_IRQn

#define Pin_COMx_TX  GPIO_Pin_2
#define Pin_COMx_RX  GPIO_Pin_3
#define GPIO_COMx    GPIOA

#define Pin_Config   GPIO_Pin_12
#define GPIO_Config  GPIOB

#define COMX_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 512)

#define TimOfWait   20
#define TimOfDelay  100

static xQueueHandle __comxQueue;

static ZigBee_Param CommMsg = {"0000", "SHUNCOM ", "1", "2", "FF", "F", "2", "2", "4", "1", "2", "1"};

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
	GPIO_Init(GPIO_COMx, &GPIO_InitStructure);				       //ZigBee模块的串口

	GPIO_InitStructure.GPIO_Pin =  Pin_Config;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_Config, &GPIO_InitStructure);				     //ZigBee模块的配置脚
	
	GPIO_SetBits(GPIO_Config, Pin_Config);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = COMx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


void StartConfig(void){
	GPIO_SetBits(GPIO_Config, Pin_Config);
	vTaskDelay(500);
	GPIO_ResetBits(GPIO_Config, Pin_Config);
}

void EndConfig(void){
	GPIO_SetBits(GPIO_Config, Pin_Config);
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

void ComxComSendStr(char *str){
	while(*str)
		ComxComSendChar(*str++);
}	

bool CommxTaskSendData(const char *dat, int len) {
	ComxTaskMsg *message = __ComxCreateMessage(TYPE_SEND_DATA, dat, len);
	if (pdTRUE != xQueueSend(__comxQueue, &message, configTICK_RATE_HZ * 5)) {
		__ComxDestroyMessage(message);
		return true;
	}
	return false;
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
char HubNode = 1;                     //1为中心节点， 2为配置选项

void USART2_IRQHandler(void) {
	unsigned char data;
	char param1, param2;
	
	if (USART_GetITStatus(COMx, USART_IT_RXNE) == RESET) {
		return;
	}
	
	data = USART_ReceiveData(COMx);
	USART_SendData(USART1, data);
	USART_ClearITPendingBit(COMx, USART_IT_RXNE);
	
	if(HubNode == 0)
		return;
	
	if(HubNode == 2){
		if ((data == 0x0A) || (data == 0x3E)){
			ComxTaskMsg *msg;
			portBASE_TYPE xHigherPriorityTaskWoken;
			if(data == 0x3E)
				buffer[bufferIndex++] = 0x3E;
			buffer[bufferIndex++] = 0;
			msg = __ComxCreateMessage(TYPE_MODIFY_DATA, (const char *)buffer, bufferIndex);		
			if (pdTRUE == xQueueSendFromISR(__comxQueue, &msg, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
			bufferIndex = 0;
		} else if (data != 0x0D) {
			buffer[bufferIndex++] = data;		
			if(bufferIndex == 20)
				bufferIndex++;
		} 
	} else if(HubNode == 1){

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
}

static void __handleRecieve(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
}

static void __handleSend(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
	if((p[0] == '1') && (strlen(p) == 1)){
		__CommInitUsart(38400);
		StartConfig();	
	}
	vTaskDelay(TimOfDelay);
	ComxComSendStr(p);
}

static void __handleModify(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	char buf[6];
	
	NorFlashRead(IPO_PARAM_CONFIG_ADDR, (short *)buf, 5);
	if(strncasecmp((const char *)buf, "FIRST", 5) != 0){               //首次配置中心节点要全部参数配置
		if(strncasecmp(p, "1.中文    2.English", 19) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr("1");
		} else if(strncasecmp(p, "请输入安全码：SHUNCOM", 21) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr("SHUNCOM");
			vTaskDelay(TimOfDelay);
			ComxComSendStr("3");
		} else if(strncasecmp(p, "节点名称:", 9) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.NODE_NAME);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("4");
		} else if(strncasecmp(p, "节点类型:", 9) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.NODE_TYPE);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("1");
		} else if(strncasecmp(p, "网络类型:", 9) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.NET_TYPE);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("5");
		} else if(strncasecmp(p, "网络ID:", 7) == 0){
			if(FrequencyDot == 1){
				sprintf(CommMsg.NET_ID, "%02X", NetID1);
			} else if(FrequencyDot == 2){
				sprintf(CommMsg.NET_ID, "%02X", NetID2);
			}
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.NET_ID);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("6");
		} else if(strncasecmp(p, "频点设置:", 9) == 0){	
			if(FrequencyDot == 1){
				sprintf(CommMsg.FREQUENCY, "%X", FrequPoint1);
			} else if(FrequencyDot == 2){
				sprintf(CommMsg.FREQUENCY, "%X", FrequPoint2);
			}				
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.FREQUENCY);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("7");
		} else if(strncasecmp(p, "地址编码:", 9) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.ADDR_CODE);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("8");
		} else if(strncasecmp(p, "发送模式:", 9) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.TX_TYPE);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("9");
		} else if(strncasecmp(p, "波特率:", 7) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.BAUDRATE);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("A");
		} else if(strncasecmp(p, "校 验:", 6) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.DATA_PARITY);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("B");
		} else if(strncasecmp(p, "数据位:", 7) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.DATA_BIT);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("F");
		} else if(strncasecmp(p, "数据源地址:", 11) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.SRC_ADR);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("E");
		} else if(strncasecmp(p, "SHUNCOM Z-BEE CONFIG:", 21) == 0){
			Ili9320TaskClear("C", 1);
		} else if(strncasecmp(p, "请选择设置参数:", 15) == 0) {
			vTaskDelay(TimOfDelay);
			ComxComSendStr("D");
			__CommInitUsart(9600);
		}	
		
		NorFlashWrite(IPO_PARAM_CONFIG_ADDR, (const short*)"FIRST", 5);
	} else {                                          //从第二次开始后可每次只配置频点和网络ID
		Node_Infor msg;
		
		if(strncasecmp(p, "1.中文    2.English", 19) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr("1");
		} else if(strncasecmp(p, "请输入安全码：SHUNCOM", 21) == 0){
			vTaskDelay(TimOfWait);
			ComxComSendStr("SHUNCOM");
			vTaskDelay(TimOfDelay);
			ComxComSendStr("5");
		} else if(strncasecmp(p, "网络ID:", 7) == 0){
			if(FrequencyDot == 1){
				msg.NetIDValue = NetID1;
				sprintf(CommMsg.NET_ID, "%02X", NetID1);
			} else if(FrequencyDot == 2){
				msg.NetIDValue = NetID2;
				sprintf(CommMsg.NET_ID, "%02X", NetID2);
			}
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.NET_ID);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("6");
		} else if(strncasecmp(p, "频点设置:", 9) == 0){	
			if(FrequencyDot == 1){
				msg.FrequValue = FrequPoint1;
				sprintf(CommMsg.FREQUENCY, "%X", FrequPoint1);
			} else if(FrequencyDot == 2){
				msg.FrequValue = FrequPoint2;
				sprintf(CommMsg.FREQUENCY, "%X", FrequPoint2);
			}				
			msg.NodePro  = Project;
			msg.GateWayOrd = GateWayID;
			msg.MaxFrequDot = NumOfFrequ;
			msg.FrequPointth = FrequencyDot;
	
			NorFlashWrite(IPO_PARAM_CONFIG_ADDR, (const short *)&msg, sizeof(Node_Infor *));        //配置结束后，把信息储存到FLASH中
			vTaskDelay(TimOfWait);
			ComxComSendStr(CommMsg.FREQUENCY);
			vTaskDelay(TimOfDelay);
			ComxComSendStr("D");
			
			__CommInitUsart(9600);
		}	
		
		
	}
	
	EndConfig();
	HubNode = 1;                //开始操作镇流器
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
	__CommInitHardware();
	__CommInitUsart(9600);
	__comxQueue = xQueueCreate(5, sizeof(ComxTaskMsg *));
	xTaskCreate(__comxTask, (signed portCHAR *) "COM", COMX_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

