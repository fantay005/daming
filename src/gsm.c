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
#include "protocol.h"
#include "misc.h"
#include "sms.h"
#include "xfs.h"
#include "zklib.h"
#include "atcmd.h"
#include "norflash.h"
#include "unicode2gbk.h"
#include "soundcontrol.h"
#include "second_datetime.h"

#define GSM_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 256)
#define GSM_GPRS_HEART_BEAT_TIME     (configTICK_RATE_HZ * 60)
#define GSM_IMEI_LENGTH              15

#define __gsmPortMalloc(size)        pvPortMalloc(size)
#define __gsmPortFree(p)             vPortFree(p)

#define  GPIO_GSM       GPIOB
#define  Pin_Restart    GPIO_Pin_0
#define  Pin_Reset	    GPIO_Pin_2


XFSspeakParam  __speakParam;

void __gsmSMSEncodeConvertToGBK(SMSInfo *info ) {
	uint8_t *gbk;

	if (info->encodeType == ENCODE_TYPE_GBK) {
		return;
	}
	gbk = Unicode2GBK((const uint8_t *)info->content, info->contentLen);
	strcpy((char *)info->content, (const char *)gbk);
	Unicode2GBKDestroy(gbk);
	info->encodeType = ENCODE_TYPE_GBK;
	info->contentLen = strlen((const char *)info->content);
}


/// GSM task message queue.
static xQueueHandle __queue;

/// Save the imei of GSM modem, filled when GSM modem start.
static char __imei[GSM_IMEI_LENGTH + 1];

const char *GsmGetIMEI(void) {
	return __imei;
}

/// Save runtime parameters for GSM task;
static GMSParameter __gsmRuntimeParameter = {"61.190.38.46", 10000, 1, 0, "0620", 1, 1};	  // 老平台服务器及端口："221.130.129.72",5555

//static GMSParameter __gsmRuntimeParameter = {"221.130.129.72", 5555, 1, 0, "0620", 1, 2};

/// Basic function for sending AT Command, need by atcmd.c.
/// \param  c    Char data to send to modem.
void ATCmdSendChar(char c) {
	USART_SendData(USART3, c);
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

/// Low level set TCP server IP and port.
//static void __setGSMserverIPLowLevel(char *ip, int port) {
//	strcpy(__gsmRuntimeParameter.serverIP, ip);
//	__gsmRuntimeParameter.serverPORT = port;
//	__storeGsmRuntimeParameter();
//}

typedef enum {
	TYPE_NONE = 0,
	TYPE_SMS_DATA,
	TYPE_RING,
	TYPE_GPRS_DATA,
	TYPE_RTC_DATA,
	TYPE_TUDE_DATA,
	TYPE_CSQ_DATA,
	TYPE_COPS_DATA,
	TYPE_SEND_TCP_DATA,
	TYPE_RESET,
	TYPE_NO_CARRIER,
	TYPE_SEND_AT,
	TYPE_SEND_SMS,
	TYPE_SET_GPRS_CONNECTION,
	TYPE_SETIP,
	TYPE_SETSPACING,
	TYPE_SETFREQU,
	TYPE_HTTP_DOWNLOAD,
	TYPE_SET_NIGHT_QUIET,
	TYPE_QUIET_TIME,
} GsmTaskMessageType;

/// Message format send to GSM task.
typedef struct {
	/// Message type.
	GsmTaskMessageType type;
	/// Message lenght.
	unsigned int length;
} GsmTaskMessage;


/// Get the data of a message.
/// \param  message    Which message to get data from.
/// \return The associate data of the message.
static inline void *__gsmGetMessageData(GsmTaskMessage *message) {
	return &message[1];
}

/// Create a message.
/// \param  type   The type of message to create.
/// \param  data   Associate this data to the message.
/// \param  len    Then lenght(byte number) of the data.
/// \return !=NULL The message which created.
/// \return ==NULL Create message failed.
GsmTaskMessage *__gsmCreateMessage(GsmTaskMessageType type, const char *dat, int len) {
	GsmTaskMessage *message = __gsmPortMalloc(ALIGNED_SIZEOF(GsmTaskMessage) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

/// Destroy a message.
/// \param  message   The message to destory.
void __gsmDestroyMessage(GsmTaskMessage *message) {
	__gsmPortFree(message);
}

int GsmTaskResetSystemAfter(int seconds) {
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_RESET, 0, 0);
	message->length = seconds;
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 5)) {
		__gsmDestroyMessage(message);
		return 0;
	}
	return 1;
}


/// Send a AT command to GSM modem.
/// \param  atcmd  AT command to send.
/// \return true   When operation append to GSM task message queue.
/// \return false  When append operation to GSM task message queue failed.
bool GsmTaskSendAtCommand(const char *atcmd) {
	int len = strlen(atcmd);
	GsmTaskMessage *message = __gsmCreateMessage(TYPE_SEND_AT, atcmd, len + 2);
	char *dat = __gsmGetMessageData(message);
	dat[len] = '\r';
	dat[len + 1] = 0;
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 5)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;

}

/// Send a AT command to GSM modem.
/// \param  atcmd  AT command to send.
/// \return true   When operation append to GSM task message queue.
/// \return false  When append operation to GSM task message queue failed.
bool GsmTaskSendSMS(const char *pdu, int len) {
    GsmTaskMessage *message;
    if(strncasecmp((const char *)pdu, "<SETIP>", 7) == 0){
	   message = __gsmCreateMessage(TYPE_SETIP, &pdu[7], len-7);
	} else if (strncasecmp((const char *)pdu, "<QUIET>", 7) == 0){
	   message = __gsmCreateMessage(TYPE_QUIET_TIME, &pdu[9], len-9);
	} else if (strncasecmp((const char *)pdu, "<SPA>", 5) == 0){
	   message = __gsmCreateMessage(TYPE_SETSPACING, &pdu[5], len-5);
	} else if (strncasecmp((const char *)pdu, "<FREQ>", 6) == 0){
	   message = __gsmCreateMessage(TYPE_SETFREQU, &pdu[6], len-6);
	} else {
	   message = __gsmCreateMessage(TYPE_SEND_SMS, pdu, len);
	}
//	char *dat = __gsmGetMessageData(message);
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;
}


bool GsmTaskSetParameter(const char *dat, int len) {
	GsmTaskMessage *message;
	  if(strncasecmp((const char *)dat, "<CTCP>", 6) == 0){
	   message = __gsmCreateMessage(TYPE_SET_GPRS_CONNECTION, &dat[6], 1);
	} else if(strncasecmp((const char *)dat, "<QUIET>", 7) == 0){
	   message = __gsmCreateMessage(TYPE_SET_NIGHT_QUIET, &dat[7], 1);
	}
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
		return false;
	}
	return true;
}


/// Send data to TCP server.
/// \param  dat    Data to send.
/// \param  len    Then length of the data.
/// \return true   When operation append to GSM task message queue.
/// \return false  When append operation to GSM task message queue failed.
bool GsmTaskSendTcpData(const char *dat, int len) {
	GsmTaskMessage *message;
	if(strncasecmp((const char *)dat, "<http>", 6) == 0){
	    message = __gsmCreateMessage(TYPE_HTTP_DOWNLOAD, &dat[6], (len - 6));
	} else {	
	    message = __gsmCreateMessage(TYPE_SEND_TCP_DATA, dat, len);
	}
	if (pdTRUE != xQueueSend(__queue, &message, configTICK_RATE_HZ * 15)) {
		__gsmDestroyMessage(message);
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
	GPIO_Init(GPIOB, &GPIO_InitStructure);				   //GSM模块的串口

    GPIO_SetBits(GPIO_GSM, Pin_Reset);
	GPIO_ResetBits(GPIO_GSM, Pin_Restart);

    GPIO_InitStructure.GPIO_Pin =  Pin_Restart | Pin_Reset;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_GSM, &GPIO_InitStructure);


	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
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
	{ "RING", TYPE_RING },
	{ "NO CARRIER", TYPE_NO_CARRIER },
	{ NULL, TYPE_NONE },
};



static char buffer[4096];
static int bufferIndex = 0;
static char isIPD = 0;
static char isSMS = 0;
static char isRTC = 0;
static char isTUDE = 0;
static char isCSQ = 0;
static char isCops = 0;
static int lenIPD;

static inline void __gmsReceiveIPDData(unsigned char data) {
	buffer[bufferIndex++] = data;
	if ((isIPD == 3) && (bufferIndex >= lenIPD + 14)) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		GsmTaskMessage *message;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_GPRS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isIPD = 0;
		bufferIndex = 0;
	}
}


static inline void __gmsReceiveSMSData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_SMS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isSMS = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static inline void __gmsReceiveRTCData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_RTC_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isRTC = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static inline void __gmsReceiveTUDEData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_TUDE_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isTUDE = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static inline void __gmsReceiveCSQData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_CSQ_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isCSQ = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}

static inline void __gmsReceiveCOPSData(unsigned char data) {
	if (data == 0x0A) {
		GsmTaskMessage *message;
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		buffer[bufferIndex++] = 0;
		message = __gsmCreateMessage(TYPE_COPS_DATA, buffer, bufferIndex);
		if (pdTRUE == xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		isCops = 0;
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
	}
}


void USART3_IRQHandler(void) {
	unsigned char data;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(USART3);
	USART_SendData(USART2, data);
	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	if (isIPD) {
		__gmsReceiveIPDData(data);
		return;
	}

	if (isSMS) {
		__gmsReceiveSMSData(data);
		return;
	}

	if (isRTC) {
		__gmsReceiveRTCData(data);
		return;
	}
	
	if (isTUDE) {
		__gmsReceiveTUDEData(data);
		return;
	}
	
	if (isCSQ) {
		__gmsReceiveCSQData(data);
		return;
	}
	
	if (isCops) {
		__gmsReceiveCOPSData(data);
		return;
	}


	if (data == 0x0A) {
		buffer[bufferIndex++] = 0;
		if (bufferIndex >= 2) {
			portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
			const GSMAutoReportMap *p;
			if (strncmp(buffer, "+CMT:", 5) == 0) {
				bufferIndex = 0;
				isSMS = 1;
			}
			for (p = __gsmAutoReportMaps; p->prefix != NULL; ++p) {
				if (strncmp(p->prefix, buffer, strlen(p->prefix)) == 0) {
					GsmTaskMessage *message = __gsmCreateMessage(p->type, buffer, bufferIndex);
					xQueueSendFromISR(__queue, &message, &xHigherPriorityTaskWoken);
					break;
				}
			}
			if (p->prefix == NULL) {
				ATCommandGotLineFromIsr(buffer, bufferIndex, &xHigherPriorityTaskWoken);
			}

			if (xHigherPriorityTaskWoken) {
				taskYIELD();
			}
		}
		bufferIndex = 0;
	} else if (data != 0x0D) {
		buffer[bufferIndex++] = data;
		if ((bufferIndex == 1) && (data == 0x02)) {
			isIPD = 1;
		}
		if (strncmp(buffer, "*PSUTTZ: ", 9) == 0) {
			bufferIndex = 0;
			isRTC = 1;
		}
		
		if (strncmp(buffer, "+CSQ: ", 6) == 0) {
			bufferIndex = 0;
			isCSQ = 1;
		}
		
		if (strncmp(buffer, "+COPS: 0,0,", 11) == 0) {
			bufferIndex = 0;
			isCops = 1;
		}
	}
}

/// Start GSM modem.
void __gsmModemStart(){
	GPIO_SetBits(GPIO_GSM, Pin_Restart);
	vTaskDelay(configTICK_RATE_HZ);
	GPIO_ResetBits(GPIO_GSM, Pin_Restart);
	vTaskDelay(configTICK_RATE_HZ);

	GPIO_ResetBits(GPIO_GSM, Pin_Reset);
	vTaskDelay(configTICK_RATE_HZ);
	GPIO_SetBits(GPIO_GSM, Pin_Reset);
	vTaskDelay(configTICK_RATE_HZ * 3);		
}

/// Check if has the GSM modem connect to a TCP server.
/// \return true   When the GSM modem has connect to a TCP server.
/// \return false  When the GSM modem dose not connect to a TCP server.


bool __gsmIsTcpConnected() {
	char *reply;
	while (1) {
		reply = ATCommand("AT+CIPSTATUS\r", "STATE:", configTICK_RATE_HZ * 2);
		if (reply == NULL) {
			return false;
		}
		if (strncmp(&reply[7], "CONNECT OK", 10) == 0) {
			AtCommandDropReplyLine(reply);
			return true;
		}
		if (strncmp(&reply[7], "TCP CONNECTING", 14) == 0) {
			AtCommandDropReplyLine(reply);
			vTaskDelay(configTICK_RATE_HZ * 5);
			continue;
		}
		if (strncmp(&reply[7], "TCP CLOSED", 10) == 0) {
			AtCommandDropReplyLine(reply);
			return false;
		}
		AtCommandDropReplyLine(reply);
		break;
	}
	return false;
}


static char flag = 0;

void choose(char *p){
	if(*p <= 40){
		(*p)++;
	} else if((*p) > 40){
		*p = 0;
	}
}
bool __gsmSendTcpDataLowLevel(const char *p, int len) {
	int i;
	char buf[18];
	char *reply;
	
  sprintf(buf, "AT+CIPSEND=%d\r", len);		  //len多大1460
	
	while (1) {	
		ATCommand(buf, NULL, configTICK_RATE_HZ / 10);
		for (i = 0; i < len; i++) {
			ATCmdSendChar(*p++);
		}
		reply = ATCommand(NULL, "DATA", configTICK_RATE_HZ / 5);
		if (reply == NULL) {
			vTaskDelay(configTICK_RATE_HZ / 2);
			continue;
		}
		choose(&flag);
	  printf("Count Start,The flag is %d.START!!\r", flag);
		if (0 == strncmp(reply, "DATA ACCEPT", 11)) {
			AtCommandDropReplyLine(reply);
			printf("Count end,The flag is %d.\r", flag);
			return true;
		}else {
			AtCommandDropReplyLine(reply);
		}
	}
}

bool __gsmCheckTcpAndConnect(const char *ip, unsigned short port) {
	char buf[44];
	char *reply;
	if (__gsmIsTcpConnected()) {
		return true;
	}

   if (!ATCommandAndCheckReply("AT+CIPSHUT\r", "SHUT", configTICK_RATE_HZ * 3)) {
		printf("AT+CIPSHUT error\r");
		return false;
	}

	sprintf(buf, "AT+CIPSTART=\"TCP\",\"%s\",%d\r", ip, port);
	reply = ATCommand(buf, "CONNECT", configTICK_RATE_HZ * 20);
	if (reply == NULL) {
		return false;
	}

	if (strncmp("CONNECT OK", reply, 10) == 0) {
		int size;
		const char *data;
		AtCommandDropReplyLine(reply);
		__gsmSendTcpDataLowLevel(data, size);
		return true;
	}
	AtCommandDropReplyLine(reply);
	return false;
}

bool __initGsmRuntime() {
	int i;
	static const int bauds[] = {115200};
	for (i = 0; i < ARRAY_MEMBER_NUMBER(bauds); ++i) {
		// 设置波特率
		printf("Init gsm baud: %d\n", bauds[i]);
		__gsmInitUsart(bauds[i]);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ / 2);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ / 2);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ / 2);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ / 2);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ / 2);
		ATCommandAndCheckReply("AT\r", "OK", configTICK_RATE_HZ / 2);

		if (ATCommandAndCheckReply("ATE0\r", "OK", configTICK_RATE_HZ * 2)) {	  //回显模式关闭
			break;
		}
	}
	if (i >= ARRAY_MEMBER_NUMBER(bauds)) {
		printf("All baud error\n");
		return false;
	}
	
	vTaskDelay(configTICK_RATE_HZ * 2);

	if (!ATCommandAndCheckReply("AT+IPR=115200\r", "OK", configTICK_RATE_HZ * 2)) {		   //设置通讯波特率
		printf("AT+IPR=115200 error\r");
		return false;
	}
	
//	 if (!ATCommandAndCheckReply("AT&F\r", "OK", configTICK_RATE_HZ * 5)) {		 //上报移动设备错误
//		printf("AT&F error\r");
//		return false;
//	}

    if (!ATCommandAndCheckReply("AT+CMEE=2\r", "OK", configTICK_RATE_HZ * 5)) {		 //上报移动设备错误
		printf("AT+CMEE error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CLTS=1\r", "OK", configTICK_RATE_HZ)) {		   //获取本地时间戳
		printf("AT+CLTS error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CIURC=1\r", "OK", configTICK_RATE_HZ * 2)) {	//初始化状态显示“CALL READY”
		printf("AT+CIURC error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CIPMUX=0\r", "OK", configTICK_RATE_HZ / 2)) {
 		printf("AT+CIPMUX error\r");
 	}

	if (!ATCommandAndCheckReply(NULL, "Call Ready", configTICK_RATE_HZ * 30)) {
		printf("Wait Call Realy timeout\n");
	}

	if (!ATCommandAndCheckReply("ATS0=0\r", "OK", configTICK_RATE_HZ * 2)) {	   //禁止自动应答
		printf("ATS0=0 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CMGF=0\r", "OK", configTICK_RATE_HZ * 2)) {		//选择短信息格式为PDU模式
		printf("AT+CMGF=0 error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CNMI=2,2,0,0,0\r", "OK", configTICK_RATE_HZ * 2)) {			//新消息显示模式选择
		printf("AT+CNMI error\r");
		return false;
	}

	if (!ATCommandAndCheckReplyUntilOK("AT+CPMS=\"SM\"\r", "+CPMS", configTICK_RATE_HZ * 10, 3)) {	   //优选消息存储器
		printf("AT+CPMS error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CMGDA=6\r", "OK", configTICK_RATE_HZ * 5)) {		   //删除PDU模式下所有短信
		printf("AT+CMGDA error\r");
		return false;
	}

	if (!ATCommandAndCheckReply("AT+CIPHEAD=0\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+CIPHEAD error\r");
		return false;
	}		   //配置接受数据时是否显示IP头

	if (!ATCommandAndCheckReply("AT+CIPSRIP=0\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+CIPSRIP error\r");
		return false;
	}		  //配置接受数据时是否显示发送方的IP地址和端口号

	if (!ATCommandAndCheckReply("AT+CIPSHOWTP=1\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+CIPSHOWTP error\r");
		return false;
	}		   //配置接受数据IP头是否显示传输协议

	if (!ATCommandAndCheckReply("AT+CIPCSGP=1,\"CMNET\"\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+CIPCSGP error\r");
  		return false;
	}			//打开GPRS连接
	
    if (!ATCommandAndCheckReply("AT+COPS?\r", "OK", configTICK_RATE_HZ)) {	 //显示当前注册的网络运营商
		printf("AT+COPS error\r");
	}
	
		if (!ATCommandAndCheckReply("AT+CIPQSEND=1\r", "OK", configTICK_RATE_HZ / 5)) {
		printf("AT+CIPQSEND error\r");
		return false;
	}	
	
	printf("SIM900A init OK.\r");
	return true;
}

void __handleSMS(GsmTaskMessage *p) {
	SMSInfo *sms;
	const char *dat = __gsmGetMessageData(p);
	sms = __gsmPortMalloc(sizeof(SMSInfo));
	printf("Gsm: got sms => %s\n", dat);
	SMSDecodePdu(dat, sms);
	printf("Gsm: sms_content=> %s\n", sms->content);
	if(sms->contentLen == 0) {
		__gsmPortFree(sms);
		return;
	}		
	ProtocolHandlerSMS(sms);
	__gsmPortFree(sms);
}

bool __gsmIsValidImei(const char *p) {
	int i;
	if (strlen(p) != GSM_IMEI_LENGTH) {
		return false;
	}

	for (i = 0; i < GSM_IMEI_LENGTH; i++) {
		if (!isdigit(p[i])) {
			return false;
		}
	}

	return true;
}

int __gsmGetImeiFromModem() {
	char *reply;	
	reply = ATCommand("AT+GSN\r", ATCMD_ANY_REPLY_PREFIX, configTICK_RATE_HZ / 2);
	if (reply == NULL) {
		return 0;
	}
	if (!__gsmIsValidImei(reply)) {
		return 0;
	}
	strcpy(__imei, reply);
	AtCommandDropReplyLine(reply);
	return 1;
}

void __handleProtocol(GsmTaskMessage *msg) {
	ProtocolHandler('1', __gsmGetMessageData(msg));
}

void __handleSendTcpDataLowLevel(GsmTaskMessage *msg) {
	__gsmSendTcpDataLowLevel(__gsmGetMessageData(msg), msg->length);
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
	unsigned short i, j=0;
	char *p = __gsmGetMessageData(msg);	 
	static const uint16_t Common_Year[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	
	static const uint16_t Leap_Year[] = {
	0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
	
	dateTime.year = (p[2] - '0') * 10 + (p[3] - '0');
	for(i=4; i<100; i++){
		if(p[i] == 0x0D){
			break;
		}
		
		if(p[i] == 0x20){
			j++;
		} else {
			continue;
		}
		
		if(j == 2){
			trans((char*)&(dateTime.month), i, p);
		} else if (j == 3){
			trans((char*)&(dateTime.date), i, p);
		} else if (j == 4){
			trans((char*)&(dateTime.hour), i, p);
			dateTime.hour= dateTime.hour + 8;
		} else if (j == 5){
			trans((char*)&(dateTime.minute), i, p);
		} else if (j == 6){
			trans((char*)&(dateTime.second), i, p);
		} 
	}
	
	i = dateTime.date;
	if(dateTime.hour >= 24) {
		i += 1;
		dateTime.hour = dateTime.hour - 24;
		dateTime.date += 1;
		if( 0 == dateTime.year / 4){
			i += Leap_Year[dateTime.month - 1];
			if(i > Leap_Year[dateTime.month]){
				dateTime.date = 1;
				dateTime.month += 1;
				if (dateTime.month > 12) {
					dateTime.month = 1;
					dateTime.year += 1;		
				}
			}
		} else {
			i += Common_Year[dateTime.month - 1];
			if(i > Common_Year[dateTime.month]){
				dateTime.date = 1;
				dateTime.month += 1;
				if (dateTime.month > 12) {
					dateTime.month = 1;
					dateTime.year += 1;		
				}
			}
		}
	}

	RtcSetTime(DateTimeToSecond(&dateTime));
}

void __handleReset(GsmTaskMessage *msg) {
	unsigned int len = msg->length;
	if (len > 100) {
		len = 100;
	}
	vTaskDelay(configTICK_RATE_HZ * len);
	while (1) {
		NVIC_SystemReset();
		vTaskDelay(configTICK_RATE_HZ);
	}
}

void __handleResetNoCarrier(GsmTaskMessage *msg) {
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_GSM, 0);
}

void __handleRING(GsmTaskMessage *msg) {
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_GSM, 1);
}


void __handleSendAtCommand(GsmTaskMessage *msg) {
	ATCommand(__gsmGetMessageData(msg), NULL, configTICK_RATE_HZ / 10);
}

void __handleSendSMS(GsmTaskMessage *msg) {
	static const char *hexTable = "0123456789ABCDEF";
	char buf[16];
	int i;
	char *p = __gsmGetMessageData(msg);
	sprintf(buf, "AT+CMGS=%d\r", msg->length - 1);
	ATCommand(buf, NULL, configTICK_RATE_HZ / 5);
	for (i = 0; i < msg->length; ++i) {
		ATCmdSendChar(hexTable[*p >> 4]);
		ATCmdSendChar(hexTable[*p & 0x0F]);
		++p;
	}
	ATCmdSendChar(0x1A);

	p = ATCommand(NULL, "OK", configTICK_RATE_HZ * 15);
	if (p != NULL) {
		AtCommandDropReplyLine(p);
		printf("Send SMS OK.\n");
	} else {
		printf("Send SMS error.\n");
	}
}


void __handleGprsConnection(GsmTaskMessage *msg) {	
}

//<SETIP>"221.130.129.72"5555
void __handleSetIP(GsmTaskMessage *msg) {
   int j = 0;
   char *dat = __gsmGetMessageData(msg);
	 if (!ATCommandAndCheckReply("AT+QICLOSE\r", "CLOSE OK", configTICK_RATE_HZ / 2)) {
		 printf("AT+QICLOSE error\r");
	 }	
	 memset(__gsmRuntimeParameter.serverIP, 0, 16);
	 if(*dat++ == 0x22){
		while(*dat != 0x22){
			 __gsmRuntimeParameter.serverIP[j++] = *dat++;
		}
		*dat++;
	 }
	 __gsmRuntimeParameter.serverPORT = atoi(dat);
}

void __handleSetSpacing(GsmTaskMessage *msg) {
}

void __handleSetFrequ(GsmTaskMessage *msg) {
}

void __handleNightQuiet(GsmTaskMessage *msg) {
}

static unsigned char Vcsq = 0;

void __handleCSQ(GsmTaskMessage *msg) {
    char *dat = __gsmGetMessageData(msg);
	  if (dat[0] == 0x20) {
			dat++;
		}
	
	  if ((dat[0] > 0x33) && (dat[0] < 0x30)){
			Vcsq = 8;
			return;
		}
		
    Vcsq = (dat[0] - '0') * 10 + dat[1] - '0';
		
	  if (Vcsq > 31) {
			Vcsq = 10;
		}
}

void __handleQuietTime(GsmTaskMessage *msg) {
}

void __handleHttpDownload(GsmTaskMessage *msg) {
}

typedef struct {
	GsmTaskMessageType type;
	void (*handlerFunc)(GsmTaskMessage *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ TYPE_SMS_DATA, __handleSMS },
	{ TYPE_RING, __handleRING },
	{ TYPE_GPRS_DATA, __handleProtocol },
	{ TYPE_SEND_TCP_DATA, __handleSendTcpDataLowLevel },
	{ TYPE_RTC_DATA, __handleM35RTC},
	{ TYPE_RESET, __handleReset },
	{ TYPE_NO_CARRIER, __handleResetNoCarrier },
  { TYPE_CSQ_DATA, __handleCSQ },
	{ TYPE_SEND_AT, __handleSendAtCommand },
	{ TYPE_SEND_SMS, __handleSendSMS },
	{ TYPE_SET_GPRS_CONNECTION, __handleGprsConnection },
	{ TYPE_SETIP, __handleSetIP },
  { TYPE_SETSPACING, __handleSetSpacing },
  { TYPE_SETFREQU, __handleSetFrequ },
	{ TYPE_HTTP_DOWNLOAD, __handleHttpDownload },
	{ TYPE_SET_NIGHT_QUIET, __handleNightQuiet },
	{ TYPE_QUIET_TIME, __handleQuietTime},
	{ TYPE_NONE, NULL },
};


static void __gsmTask(void *parameter) {
	portBASE_TYPE rc;
	GsmTaskMessage *message;
	portTickType lastT = 0;
	while (1) {
		printf("Gsm start\n");
		__gsmModemStart();
		if (__initGsmRuntime()) {
			break;
		}		   
	}
	while (!__gsmGetImeiFromModem()) {
		vTaskDelay(configTICK_RATE_HZ);
	}
 

	for (;;) {
		printf("Gsm: loop again\n");
		rc = xQueueReceive(__queue, &message, configTICK_RATE_HZ * 10);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != TYPE_NONE; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			__gsmDestroyMessage(message);
		} else {
			portTickType curT;			
			curT = xTaskGetTickCount();	
							
			if(__gsmRuntimeParameter.isonTCP == 0){
			   continue;
			}										
			
			if (0 == __gsmCheckTcpAndConnect(__gsmRuntimeParameter.serverIP, __gsmRuntimeParameter.serverPORT)) {
				printf("Gsm: Connect TCP error\n");
			} else if ((curT - lastT) >= GSM_GPRS_HEART_BEAT_TIME) {
				lastT = curT;
			} 		
		}
	}
}

void GSMInit(void) {
	ATCommandRuntimeInit();
	__gsmInitHardware();
	__queue = xQueueCreate(5, sizeof( GsmTaskMessage *));
	xTaskCreate(__gsmTask, (signed portCHAR *) "GSM", GSM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}
