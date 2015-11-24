#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stdlib.h"
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


#define COMX_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 512)

#define COMx         USART2
#define COMx_IRQn    USART2_IRQn

#define Pin_COMx_TX  GPIO_Pin_2
#define Pin_COMx_RX  GPIO_Pin_3
#define GPIO_COMx    GPIOA

#define Pin_Config   GPIO_Pin_5
#define GPIO_Config  GPIOC

#define TimOfWait   120
#define TimOfDelay  100

static xQueueHandle __comxQueue;

extern char NodeAble;

typedef struct{
	char  MAC_ADDR[5];
	char  NODE_NAME[9];
  char  NODE_TYPE[2];
	char  NET_TYPE[2];
	char  NET_ID[3];
	char  FREQUENCY[2];
	char  ADDR_CODE[2];
	char  TX_TYPE[2];
	char  BAUDRATE[2];
	char  DATA_PARITY[2];
	char  DATA_BIT[2];
	char  SRC_ADR[2];
}ZigBee_Param;

typedef struct{
	unsigned char  STATE[10];
	unsigned char  DIM[5];
	unsigned char  INPUT_VOL[5];
	unsigned char  INPUT_CUR[5];
	unsigned char  INPUT_POW[5];
	unsigned char  LIGHT_VOL[5];
	unsigned char  PFC_VOL[5];
	unsigned char  TEMP[5];
	unsigned char  TIME[8];
}BSN_Data;


static ZigBee_Param CommMsg = {"0AAA", "SHUNCOM ", "1", "2", "FF", "F", "2", "2", "4", "1", "2", "3"};

static void ZigBeeParamInit(void){
	sprintf(CommMsg.NET_ID, "FF");
	sprintf(CommMsg.FREQUENCY, "F");
	sprintf(CommMsg.SRC_ADR, "3");
}

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
}

void OpenComxInterrupt(void){	
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = COMx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
}

void CloseComxInterrupt(void){	
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = COMx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	
	NVIC_Init(&NVIC_InitStructure);
}

void StartConfig(void){
	__CommInitUsart(38400);
	GPIO_ResetBits(GPIO_Config, Pin_Config);
}

void EndConfig(void){
	GPIO_SetBits(GPIO_Config, Pin_Config);
}

typedef enum{
	TYPE_PRINTF_DATA,
	TYPE_RECIEVE_DATA,
	TYPE_SEND_DATA,
	TYPE_MODIFY_DATA,
	TYPE_CONFIG_DATA,
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

void ComxComSendLenth(char *str, unsigned char len){
	unsigned char i;
	
	for(i = 0; i < len; i++)
		ComxComSendChar(*str++);
}	

bool CommxTaskSendData(const char *dat, unsigned char len) {
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

bool ComxConfigData(const char *dat, int len) {
	ComxTaskMsg *message = __ComxCreateMessage(TYPE_CONFIG_DATA, dat, len);
	if (pdTRUE != xQueueSend(__comxQueue, &message, configTICK_RATE_HZ * 5)) {
		__ComxDestroyMessage(message);
		return true;
	}
	return false;
}

static unsigned char bufferIndex;
static unsigned char buffer[255];
static unsigned char LenZIGB;         //接收到帧的数据长度
static unsigned char Illegal = 0;     //接收到的数据是否为合法帧
unsigned short Addr = 0;              //乱叫的ZigBee地址
static unsigned short Sequ = 0;       //接收到非法字符的个数
char HubNode = 3;                     //1为接收模式， 2为配置选项，3为HEX源地址输出

static void  IsIllegal(unsigned char p){

	Illegal = 1;
	Sequ++;
	if(Sequ == 1){
		Addr = p << 8;
	} else if(Sequ == 2){
		Addr |= p;
	}
}

static bool IsVisibleChar(unsigned char p){
	if((p > 0x29) && (p < 0x3A))
		return false;
	if((p > 0x40) && (p < 0x47))
		return false;
	
	return true;
}

void USART2_IRQHandler(void) {
	unsigned char data;
	char param1, param2;
	
	if (USART_GetITStatus(COMx, USART_IT_RXNE) == RESET) {
		return;
	}
	
	data = USART_ReceiveData(COMx);
	USART_SendData(USART1, data);
	USART_ClearITPendingBit(COMx, USART_IT_RXNE);
	

	if(HubNode == 2){                                   //当收到配置信息时
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
		} 
	} else if(HubNode == 1){                             //当收到手持设备发送的中心节点频点和网络ID时

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
	} else if(HubNode == 3){                             //当源地址输出设置为HEX时，接收到的数据
		char HexToChar[] = {"0123456789ABCDEF"};
		
		if(data == 0x03){
			data = 0x03;
		}
		
		if(((bufferIndex == 200) || (data == 0x03)) && (bufferIndex > 3)){
			ComxTaskMsg *msg;
			portBASE_TYPE xHigherPriorityTaskWoken;
			
			buffer[bufferIndex++] = data;
			if(Illegal == 0){
				Sequ = 0;
				bufferIndex = 0;
				return;
			}
			msg = __ComxCreateMessage(TYPE_PRINTF_DATA, (const char *)buffer, bufferIndex);		
			if (pdTRUE == xQueueSendFromISR(__comxQueue, &msg, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
			
			Illegal = 0;
			Sequ = 0;
			bufferIndex = 0;
		} else if(bufferIndex == 0){                      //ZigBee地址不大于0x1000
			if(data > 0x05)
				return;
			buffer[bufferIndex++] = data;
		} else if(bufferIndex == 2){                      //判断是否为合法字节
			if(data != 0x02){
				buffer[0] = buffer[1];
				bufferIndex = 1;
			}
				
			if(buffer[0] > 0x05){
				buffer[0] = data;
				bufferIndex = 0;
			}
				
			if(buffer[0] > 0x05){
				bufferIndex = 0;
				return;
			}
			
			buffer[bufferIndex++] = data;
		} else if(bufferIndex ==  3){                //当收到的第一个合法字节后	
			
			buffer[bufferIndex] = data;
			if(buffer[3] != HexToChar[buffer[0] >> 4 & 0xF])
				IsIllegal(data);
			bufferIndex++;
		} else if(bufferIndex == 4){
			
			buffer[bufferIndex] = data;
			if((buffer[4] != HexToChar[buffer[0] & 0xF]) || Illegal)
				IsIllegal(data);
			bufferIndex++;
		} else if(bufferIndex == 5){
			
			buffer[bufferIndex] = data;
			if((buffer[5] != HexToChar[buffer[1] >> 4 & 0xF]) || Illegal)
				IsIllegal(data);
			bufferIndex++;
		} else if(bufferIndex == 6){
			
			buffer[bufferIndex] = data;
			if((buffer[6] != HexToChar[buffer[1] & 0xF]) || Illegal)
				IsIllegal(data);
			bufferIndex++;
		} else if(bufferIndex > 6){                                         //先全部接收
			
			buffer[bufferIndex] = data;
			if((IsVisibleChar(data)) || Illegal)
				IsIllegal(data);
		}	 else {
			buffer[bufferIndex++] = data;
		}
	}	
}


static void __handleSend(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	unsigned char len = msg->length;
	
	if((p[0] == '1') && (len == 1)){
		StartConfig();	
	}
	vTaskDelay(TimOfDelay);
	ComxComSendLenth(p, len);
}

extern void ConfigEnd(void);

static void __handleModify(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
	if(strncasecmp(p, "1.中文    2.English", 19) == 0){
		vTaskDelay(TimOfWait);
		ComxComSendStr("1");
	
	} else if(strncasecmp(p, "请输入安全码：SHUNCOM", 21) == 0){
		vTaskDelay(TimOfWait);
		ComxComSendStr("SHUNCOM");
		vTaskDelay(TimOfDelay);
		ComxComSendStr("3");
	} else if(strncasecmp(p, "节点地址:", 9) == 0){
		vTaskDelay(TimOfWait);
		ComxComSendStr(CommMsg.MAC_ADDR);
		vTaskDelay(TimOfDelay);
		ComxComSendStr("4");
	} else if(strncasecmp(p, "节点名称:", 9) == 0){
		vTaskDelay(TimOfWait);
		ComxComSendStr(CommMsg.NODE_NAME);
		vTaskDelay(TimOfDelay);
		ComxComSendStr("1");
	} else if(strncasecmp(p, "节点类型:", 9) == 0){
		vTaskDelay(TimOfWait); 
		ComxComSendStr(CommMsg.NODE_TYPE);
		vTaskDelay(TimOfDelay);
		ComxComSendStr("2");
	} else if(strncasecmp(p, "网络类型:", 9) == 0){
		vTaskDelay(TimOfWait);
		ComxComSendStr(CommMsg.NET_TYPE);
		vTaskDelay(TimOfDelay);
		ComxComSendStr("5");
	} else if(strncasecmp(p, "网络ID:", 7) == 0){		
		vTaskDelay(TimOfWait);
		ComxComSendStr(CommMsg.NET_ID);
		vTaskDelay(TimOfDelay);
		ComxComSendStr("6");
	} else if(strncasecmp(p, "频点设置:", 9) == 0){	
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
		vTaskDelay(TimOfDelay);
		EndConfig();
		vTaskDelay(TimOfDelay);
		ComxComSendStr("D");
		vTaskDelay(TimOfDelay * 3);
		__CommInitUsart(9600);
		if(strncmp(CommMsg.SRC_ADR, "1", 1) == 0)               
			HubNode = 1;                //开始操作镇流器
		else                          //开启源地址输出
			HubNode = 3;                //HEX源地址输出
		
		ConfigEnd();
		ZigBeeParamInit();
	}	
}

extern void ConfigStart(void);

static void __handleRecieve(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	const char file[13] = {"test.txt"};
	
	SDTaskHandleCreateFile(file, strlen(file) + 1);	
	if((p[5] != 'A') || (p[6] != 'A'))
		return;
	
	if((p[7] != '0') || (p[8] != '3'))
		return;
	
	ConfigStart();
	
	CommMsg.NODE_TYPE[0] = '1';
	
	CommMsg.NET_ID[0] = p[10];
	CommMsg.NET_ID[1] = p[11];
	
	CommMsg.FREQUENCY[0] = p[9];
	
	HubNode = 2;
	StartConfig();
	vTaskDelay(TimOfDelay);
	ComxComSendStr("1");
}

static void __handleConfig(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	const char folder[8] = {"test"};
	
	SDTaskHandleCreateFolder(folder, strlen(folder) + 1);
	
	CommMsg.NODE_TYPE[0] = '3';
	
	CommMsg.NET_ID[0] = p[1];
	CommMsg.NET_ID[1] = p[2];
	
	CommMsg.FREQUENCY[0] = p[0];
	
	CommMsg.SRC_ADR[0] = '1';
	
	HubNode = 2;
	StartConfig();
	vTaskDelay(TimOfDelay);
	ComxComSendStr("1");
}

static void __handlePrintf(ComxTaskMsg *msg){
	char *p = __ComxGetMsgData(msg);
	
	SDTaskHandleWriteFile(p, msg->length);	
}

typedef struct {
	ComxTaskMessageType type;
	void (*handlerFunc)(ComxTaskMsg *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_PRINTF_DATA, __handlePrintf },
	{ TYPE_RECIEVE_DATA, __handleRecieve },
	{ TYPE_SEND_DATA, __handleSend },
	{ TYPE_MODIFY_DATA, __handleModify },
	{ TYPE_CONFIG_DATA, __handleConfig },
	{ TYPE_NONE, NULL },
};

static void __comxTask(void *parameter) {
	portBASE_TYPE rc;
	ComxTaskMsg *message;

	for (;;) {
		rc = xQueueReceive(__comxQueue, &message, configTICK_RATE_HZ * 2);
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
	OpenComxInterrupt();
	__CommInitUsart(9600);
	__comxQueue = xQueueCreate(20, sizeof(ComxTaskMsg *));
	xTaskCreate(__comxTask, (signed portCHAR *) "COM", COMX_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

