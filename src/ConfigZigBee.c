#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include "zklib.h"
#include "CommZigBee.h"
#include "ili9320.h"
#include "display.h"
#include "ConfigZigbee.h"
#include "key.h"
#include "sdcard.h"

#define SERx         USART3
#define SERx_IRQn    USART3_IRQn

#define Pin_SERx_TX  GPIO_Pin_10
#define Pin_SERx_RX  GPIO_Pin_11
#define GPIO_SERx    GPIOB

#define CONFIG_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 512)

#define waitTime    20
#define delayTime   100

static xQueueHandle __ConfigQueue;

static ZigBee_Param ConfigMsg = {"0001", "SHUNCOM ", "3", "2", "FF", "F", "2", "2", "4", "1", "2", "1"};

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

static void ConfigComSendChar(char c){
	USART_SendData(SERx, c);
	while (USART_GetFlagStatus(SERx, USART_FLAG_TXE) == RESET);
}

void ConfigComSendStr(char *str){
	while(*str)
		ConfigComSendChar(*str++);
}	

bool ConfigTaskSendData(const char *dat, int len) {
	ConfigTaskMsg *message = __ConfigCreateMessage(CONFIG_SEND_DATA, dat, len);
	if (pdTRUE != xQueueSend(__ConfigQueue, &message, configTICK_RATE_HZ * 5)) {
		__ConfigDestroyMessage(message);
		return true;
	}
	return false;
}

static unsigned char bufferIndex;
static unsigned char buffer[255];

extern char Com3IsOK(void);   //判断配置模式下是否为快速设置或高级设置状态

void USART3_IRQHandler(void) {
	unsigned char data;
	
	if (USART_GetITStatus(SERx, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(SERx);
	USART_SendData(USART1, data);
	USART_ClearITPendingBit(SERx, USART_IT_RXNE);

	if(!Com3IsOK())
		return;
	if ((data == 0x0A) || (data == 0x3E)){
		ConfigTaskMsg *msg;
		portBASE_TYPE xHigherPriorityTaskWoken;
		if(data == 0x3E)
			buffer[bufferIndex++] = 0x3E;
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

static char Open_DisPlay = 0;            //打开显示标志位

static void __TaskHandleRecieve(ConfigTaskMsg *msg){
	char *p = __ConfigGetMsgData(msg);
	
	if(Com3IsOK() == 2){
		Ili9320TaskOrderDis(p, strlen(p) + 1);
		
	} else if(Com3IsOK() == 3){
		if(!Config_Enable)
				return;
		if(strncasecmp(p, "1.中文    2.English", 19) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr("1");
		} else if(strncasecmp(p, "请输入安全码：SHUNCOM", 21) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr("SHUNCOM");
			vTaskDelay(delayTime);
			ConfigComSendStr("SHUNCOM");
		} else if(strncasecmp(p, "SHUNCOM Z-BEE CONFIG:", 21) == 0){
			Open_DisPlay = 1;
			Ili9320TaskClear("C", 1);
			Config_Enable = 2;                  //配置地址成功，进入收尾阶段
		} else if((strncasecmp(p, "请选择设置参数:", 15) == 0) && (Config_Enable == 2)){
			Open_DisPlay = 0;		
			Config_Enable = 0;                  //配置地址成功，暂停配置
			vTaskDelay(delayTime);
			ConfigComSendStr("D");
		}	
		
		if(Open_DisPlay){
			Ili9320TaskOrderDis(p, strlen(p) + 1);
		}
			
	} else if(Com3IsOK() == 1){
		if(!Config_Enable)
				return;

		if(strncasecmp(p, "1.中文    2.English", 19) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr("1");
		} else if(strncasecmp(p, "请输入安全码：SHUNCOM", 21) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr("SHUNCOM");
			vTaskDelay(delayTime);
			ConfigComSendStr("3");
		} else if(strncasecmp(p, "节点地址:", 9) == 0){
			if(HexSwitchDec){
				if((ZigBAddr > 0x2000) && (ZigBAddr < 0x3000))
					ZigBAddr = ZigBAddr - 0x2000;
				sprintf(ConfigMsg.MAC_ADDR, "%04X", ZigBAddr);
			} else {
				if((ZigBAddr > 2000) && (ZigBAddr < 3000))
					ZigBAddr = ZigBAddr - 2000;
				sprintf(ConfigMsg.MAC_ADDR, "%04d", ZigBAddr);
			}
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.MAC_ADDR);
			vTaskDelay(delayTime);
			ConfigComSendStr("2");
		} else if(strncasecmp(p, "节点名称:", 9) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.NODE_NAME);
			vTaskDelay(delayTime);
			ConfigComSendStr("4");
		} else if(strncasecmp(p, "节点类型:", 9) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.NODE_TYPE);
			vTaskDelay(delayTime);
			ConfigComSendStr("1");
		} else if(strncasecmp(p, "网络类型:", 9) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.NET_TYPE);
			vTaskDelay(delayTime);
			ConfigComSendStr("5");
		} else if(strncasecmp(p, "网络ID:", 7) == 0){
			if(FrequencyDot == 1){
				sprintf(ConfigMsg.NET_ID, "%02X", NetID1);
			} else if(FrequencyDot == 2){
				sprintf(ConfigMsg.NET_ID, "%02X", NetID2);
			}
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.NET_ID);
			vTaskDelay(delayTime);
			ConfigComSendStr("6");
		} else if(strncasecmp(p, "频点设置:", 9) == 0){	
			if(FrequencyDot == 1){
				sprintf(ConfigMsg.FREQUENCY, "%X", FrequPoint1);
			} else if(FrequencyDot == 2){
				sprintf(ConfigMsg.FREQUENCY, "%X", FrequPoint2);
			}				
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.FREQUENCY);
			vTaskDelay(delayTime);
			ConfigComSendStr("7");
		} else if(strncasecmp(p, "地址编码:", 9) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.ADDR_CODE);
			vTaskDelay(delayTime);
			ConfigComSendStr("8");
		} else if(strncasecmp(p, "发送模式:", 9) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.TX_TYPE);
			vTaskDelay(delayTime);
			ConfigComSendStr("9");
		} else if(strncasecmp(p, "波特率:", 7) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.BAUDRATE);
			vTaskDelay(delayTime);
			ConfigComSendStr("A");
		} else if(strncasecmp(p, "校 验:", 6) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.DATA_PARITY);
			vTaskDelay(delayTime);
			ConfigComSendStr("B");
		} else if(strncasecmp(p, "数据位:", 7) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.DATA_BIT);
			vTaskDelay(delayTime);
			ConfigComSendStr("F");
		} else if(strncasecmp(p, "数据源地址:", 11) == 0){
			vTaskDelay(waitTime);
			ConfigComSendStr(ConfigMsg.SRC_ADR);
			vTaskDelay(delayTime);
			ConfigComSendStr("E");
		} else if(strncasecmp(p, "SHUNCOM Z-BEE CONFIG:", 21) == 0){
			Open_DisPlay = 1;
			Ili9320TaskClear("C", 1);
			Config_Enable = 2;                  //配置地址成功，进入收尾阶段
		} else if((strncasecmp(p, "请选择设置参数:", 15) == 0) && (Config_Enable == 2)){
			Open_DisPlay = 0;		
			Config_Enable = 0;                  //配置地址成功，暂停配置
			vTaskDelay(delayTime);
			ConfigComSendStr("D");
			ZigBAddr++;                         //地址自动加一	
		}	
		
		if(Open_DisPlay){
			Ili9320TaskOrderDis(p, strlen(p) + 1);
		}
		
	}
}

static void __TaskHandleSend(ConfigTaskMsg *msg){
	char *p = __ConfigGetMsgData(msg);
	Ili9320TaskOrderDis(p, strlen(p) + 1);
	ConfigComSendStr(p);
}

typedef struct {
	ConfigTaskMessageType type;
	void (*handlerFunc)(ConfigTaskMsg *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ CONFIG_RECIEVE_DATA, __TaskHandleRecieve },
	{ CONFIG_SEND_DATA, __TaskHandleSend },
//	{ CONFIG_MODIFY_DATA, __TaskHandleModify},
	{ CONFIG_USELESS_DATA, NULL },
};

static void __ConfigTask(void *parameter) {
	portBASE_TYPE rc;
	ConfigTaskMsg *message;

	for (;;) {
	//	printf("Config: loop again\n");
		rc = xQueueReceive(__ConfigQueue, &message, configTICK_RATE_HZ );
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
	__ConfigQueue = xQueueCreate(30, sizeof(ConfigTaskMsg *));
	xTaskCreate(__ConfigTask, (signed portCHAR *) "CONFIG", CONFIG_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

