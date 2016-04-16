#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "second_datetime.h"
#include "rtc.h"
#include "misc.h"
#include "shuncom.h"
#include "protocol.h"
#include "zklib.h"
#include "norflash.h"
#include "gsm.h"

#define ZIGBEE_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 1024 * 40)

#define COMx      USART1
#define COMx_IRQn USART1_IRQn

#define COM_TX     GPIO_Pin_9
#define COM_RX     GPIO_Pin_10
#define GPIO_COM   GPIOA

static xQueueHandle __ZigbeeQueue;

static char WAITFLAG = 0;

static void SZ05_ADV_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  COM_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_COM, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = COM_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_COM, &GPIO_InitStructure);				                   //ZIGBEE模块的串口
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = COMx_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void Zibee_Baud_CFG(int baud){
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

typedef enum{
	TYPE_IOT_LAMPPARAM_DOWNLOAD, /*0x02，镇流器灯参下载*/
	TYPE_IOT_STRATEGU_DOWNLOAD,  /*0x03，镇流器策略下载*/
	TYPE_IOT_LAMP_DIMMING,       /*0x04，灯调光控制*/
	TYPE_IOT_LAMP_CONTROL,       /*0x05，灯开关控制*/
	TYPE_IOT_TIMING,	           /*0x0B，镇流器校时*/
	TYPE_IOT_RECIEVE_DATA,       /*网关接收到的镇流器数据*/
	TYPE_IOT_SEND_DATA,          /*网关向镇流器发送数据*/
	TYPE_IOT_NONE,               /**/
}ZigbTaskMsgType;

typedef struct {
	ZigbTaskMsgType type;
	unsigned char length;
	char infor[200];
} ZigbTaskMsg;

static void *ZigbtaskApplyMemory(int size){
	return pvPortMalloc(size);
}

static void ZigbTaskFreeMemory(void *p){
	vPortFree(p);
}

static inline void *__ZigbGetMsgData(ZigbTaskMsg *message) {
	return message->infor;
}

static char buffer[255];
static char bufferIndex = 0;
static char isPROTOC = 0;
static char LenZIGB = 0;

void USART1_IRQHandler(void) {
	unsigned char data;
	char param1, param2;
	
	if (USART_GetITStatus(COMx, USART_IT_RXNE) == RESET) {
		return;
	}

	data = USART_ReceiveData(COMx);
	USART_SendData(UART5, data);
	USART_ClearITPendingBit(COMx, USART_IT_RXNE);

	if(isPROTOC){
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
		  isPROTOC = 0;
			LenZIGB = 0;
		}
	}
	
	if ((bufferIndex == (LenZIGB + 12)) && (data == 0x03)){
		ZigbTaskMsg msg;
		portBASE_TYPE xHigherPriorityTaskWoken;
		buffer[bufferIndex++] = 0;
		
		msg.type = TYPE_IOT_RECIEVE_DATA;
		msg.length = bufferIndex;
		memcpy(msg.infor, buffer, bufferIndex);
		
		if (pdTRUE == xQueueSendFromISR(__ZigbeeQueue, &msg, &xHigherPriorityTaskWoken)) {
			if (xHigherPriorityTaskWoken) {
				portYIELD();
			}
		}
		bufferIndex = 0;
		isPROTOC = 0;
		LenZIGB = 0;
		WAITFLAG = 1;
	} else if(bufferIndex > (LenZIGB + 12)){
		bufferIndex = 0;
		isPROTOC = 0;
		LenZIGB = 0;
	} else if (data == 0x02){
		bufferIndex = 0;
		buffer[bufferIndex++] = data;
		isPROTOC = 1;
	}
}

static void SZ05SendChar(char c) {
	USART_SendData(COMx, c);
	while (USART_GetFlagStatus(COMx, USART_FLAG_TXE) == RESET);
}

void SZ05SendString(char *str){
	while(*str){
		SZ05SendChar(*str++);
	}
}

void ZigBeeSendStrLen(char *str, unsigned char lenth){
	unsigned char i;
	
	for(i = 0; i < lenth; i++)
		SZ05SendChar(*str++);
}

extern unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);

char BSNinfor[600][40];           /*保存所有镇流器当前数据*/

SEND_STATUS ZigbTaskSendData(const char *dat, int len) {
	int i, j;
	unsigned short addr;
	char hextable[] = "0123456789ABCDEF";
	unsigned char *buf, tmp[5], size;
	Lightparam k;
	char build[4] = {'B', '0' , '0' , '0'};
	GMSParameter g;
	
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);		
	
#if defined(__HEXADDRESS__)
	addr = (*dat >> 4) * 0x1000 + (*dat & 0x0F) * 0x100 + (*(dat + 1) >> 4) * 0x10 + (*(dat + 1) & 0x0F);
#else
	addr = (*dat >> 4) * 1000 + (*dat & 0x0F) * 100 + (*(dat + 1) >> 4) * 10 + (*(dat + 1) & 0x0F);
#endif
	
	NorFlashRead(NORFLASH_BALLAST_BASE + addr * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);	
	
	k.UpdataTime = RtcGetTime();
	
	if((k.Loop > '8') || (k.Loop < '0')){
		printf("This loop = %d, have error.\r\n", (k.Loop - '0'));
		printf("This addr = %04d.\r\n", addr);
		return RTOS_ERROR;
	}
	
	for(i = 0; i < 3; i++){		
		ZigBeeSendStrLen((char *)dat, len);

		for(j = 0; j < 50; j++){
			if(WAITFLAG == 0){
				vTaskDelay(configTICK_RATE_HZ / 50);
				if(j >= 49){
					WAITFLAG = 2;
//					if(k.CommState == 0x18)
//						i = 2;
				}
			} else {
				break;
			}
		}

		if(WAITFLAG == 1){
			i = 3;
			WAITFLAG = 0;	
			return COM_SUCCESS;
		}
		if((WAITFLAG == 2) && (i != 2)) {
			WAITFLAG = 0;
			continue;
		}
		if((WAITFLAG == 2) && (i == 2)){	
			char mess[45] = {0};
			
			if(k.CommState == 0x18){		
				char temp[32] = {0};
			
				memset(temp, '0', 30);
				sprintf(BSNinfor[addr], "%04d%04d%s", addr, 18, temp);
				
				WAITFLAG = 0;				
				return STATUS_FIT;
			}		
				
			k.CommState = 0x18;
			k.InputPower = 0;
			NorFlashWrite(NORFLASH_BALLAST_BASE + addr * NORFLASH_SECTOR_SIZE, (const short *)&k, (sizeof(Lightparam) + 1) / 2);
			
			tmp[0] = hextable[k.CommState >> 4];
			tmp[1] = hextable[k.CommState & 0x0F];
			
			memset(mess, '0', 30);
			sprintf(BSNinfor[addr], "%04d%04d%s", addr, 18, mess);
			
			memset(mess, '0', 43);
			memcpy(mess, build, 4);
			memcpy((mess + 4), (dat + 3), 4);
			memcpy((mess + 4 + 4 + 2), tmp, 2);
			mess[38 + 4] = 0;	
			
			buf = ProtocolRespond(g.GWAddr, (unsigned char *)(dat + 7), (const char *)mess, &size);
			GsmTaskSendTcpData((const char *)buf, size);
			
			WAITFLAG = 0;
			return COM_FAIL;
		}
	}
	return HANDLE_ERROR;
}

static void ZigbeeHandleLightParam(FrameHeader *header, unsigned char CheckByte, const char *p){
}

static void ZigbeeHandleStrategy(FrameHeader *header, unsigned char CheckByte, const char *p){
	
}

static char Have_Param_Flag = 0;

extern const short *LightZigbAddr(void);

extern unsigned char *DayToSunshine(void);

extern unsigned char *DayToNight(void);

static void ZigbeeHandleReadBSNData(FrameHeader *header, unsigned char CheckByte, const char *p){
	int i, instd, hexAddr;
	unsigned char *buf, space[3], addr[5], SyncFlag[13], *msg, size;
	unsigned short *ret, compare;
	GMSParameter g;
	Lightparam k;
	StrategyParam s;
	ZigbTaskMsg message;
	
	char StateFlag = 0x01;   /*软关闭*/
	
	char OpenBuf[] = {0xFF, 0xFF, 0x02, 0x46, 0x46, 0x46, 0x46, 0x30, 0x35, 0x30, 0x35, 0x41, 
											0x30, 0x30, 0x30, 0x30, 0x34, 0x33, 0x03};
	char CloseBuf[] = {0xFF, 0xFF, 0x02, 0x46, 0x46, 0x46, 0x46, 0x30, 0x35, 0x30, 0x35, 0x41,
											0x30, 0x30, 0x30, 0x31, 0x34, 0x32, 0x03};
			
  msg = ZigbtaskApplyMemory(35);											
	sscanf((const char *)header, "%*1s%4s", addr);
	i = atoi((const char *)addr);
	sscanf(p, "%*9s%34s", msg);
	sprintf(BSNinfor[i], "%s%s", addr, msg);    /*保存接收到的镇流器数据*/	
											
	ZigbTaskFreeMemory(msg);                 									
										
	
	sscanf(p, "%*1s%4s", addr);		
	instd = atoi((const char *)addr);
	hexAddr = strtol((const char *)addr, NULL, 16);

	NorFlashRead(NORFLASH_BALLAST_BASE + instd * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
	
	k.UpdataTime = RtcGetTime();

	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);	
	
	{                                           /*网关轮询镇流器数据*/
		DateTime dateTime;
		uint32_t second;
		unsigned char time_m, time_d, time_w;
		
		
		sscanf(p, "%*43s%12s", SyncFlag);
		
		if((k.Loop <= '8') || (k.Loop >= '0')) {
			Have_Param_Flag = 1;   
		}
		
		sscanf(p, "%*43s%12s", SyncFlag);
		
		if((strncmp((const char *)k.TimeOfSYNC, (const char *)SyncFlag, 12) != 0) && (Have_Param_Flag == 1)){     /*灯参数同步标识比较*/	
			
			NorFlashRead(NORFLASH_BALLAST_BASE + instd * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			msg = ZigbtaskApplyMemory(26 + 1);
	
			sscanf((const char *)k.TimeOfSYNC, "%12s", msg);
			sscanf((const char *)k.NorminalPower, "%4s", msg + 12);
			msg[16] = k.Loop;
			sscanf((const char *)k.LightPole, "%4s", msg + 17);
			msg[21] = k.LightSourceType;
			msg[22] = k.LoadPhaseLine;
			sscanf((const char *)k.Attribute, "%2s", msg + 23);
			msg[25] = 0;
		
			#if defined(__HEXADDRESS__)
					sprintf((char *)addr, "%04X", instd);
			#else				
					sprintf((char *)addr, "%04d", instd);
			#endif	
		
			buf = DataSendToBSN((unsigned char *)"02", addr, (const char *)msg, &size);
			ZigbTaskFreeMemory(msg);
	
			message.type = TYPE_IOT_SEND_DATA;
			message.length = size;
			memcpy(message.infor, buf, size);
				
			xQueueSend(__ZigbeeQueue, &message, configTICK_RATE_HZ); 

		}
		
		ret = (unsigned short*)LightZigbAddr();
		while(*ret){
			if((*ret++) == instd){
				StateFlag = 0x02;   /*主运行*/
				break;
			}	
		}
		
		sscanf(p, "%*11s%2s", space);
		i = atoi((const char *)space);  /*运行状态*/

		if((i != StateFlag) && (*LightZigbAddr() != 0)){	
			if(StateFlag == 0x01){
				CloseBuf[0] = (hexAddr >> 8) & 0xFF;
				CloseBuf[1] = hexAddr & 0xFF;
				for(i = 0; i < 19; i++){
					SZ05SendChar(CloseBuf[i]);
				}
				vTaskDelay(configTICK_RATE_HZ / 10);
			} else if(StateFlag == 0x02){
				OpenBuf[0] = (hexAddr >> 8) & 0xFF;
				OpenBuf[1] = hexAddr & 0xFF;
				for(i = 0; i < 19; i++){
					SZ05SendChar(OpenBuf[i]);
				}
				vTaskDelay(configTICK_RATE_HZ / 10);
			}
		}

		Have_Param_Flag = 0;
		sscanf(p, "%*55s%12s", SyncFlag);
		NorFlashRead(NORFLASH_STRATEGY_ADDR, (short *)&s, (sizeof(StrategyParam) + 1) / 2);
		
		if((s.SYNCTINE[0] <= '9') && (s.SYNCTINE[0] >= '0')) {
			Have_Param_Flag = 1;
		}

		if((strncmp((const char *)s.SYNCTINE, (const char *)"160315000000", 12) != 0) && (Have_Param_Flag == 1)){     /*策略同步标识比较*/
			msg = ZigbtaskApplyMemory(47 + 1);
			
			sscanf((const char *)"160315000000", "%12s", msg);
			sscanf((const char *)"01", "%2s", msg + 12);
			msg[14] = '1';
		
			sscanf((const char *)"0FFF", "%4s", msg + 15);
			sscanf((const char *)"64", "%2s", msg + 19);
			msg[21] = 0;
		
//			if ((s.DimmingNOS - '0') == 2){
//				sscanf((const char *)s.SecondDCTime, "%4s", msg + 21);
//				sscanf((const char *)s.SecondDPVal, "%2s", msg + 25);	
//				msg[27] = 0;
//			}
//			
//			if ((s.DimmingNOS - '0') == 3){
//				sscanf((const char *)s.ThirdDCTime, "%4s", msg + 27);
//				sscanf((const char *)s.ThirdDPVal, "%2s", msg + 31);
//				msg[33] = 0;
//			}
//			
//			if ((s.DimmingNOS - '0') == 4){
//				sscanf((const char *)s.FourthDCTime, "%4s", msg + 33);
//				sscanf((const char *)s.FourthDPVal, "%2s", msg + 37);
//				msg[39] = 0;
//			}
//					
//			if ((s.DimmingNOS - '0') == 5){
//				sscanf((const char *)s.FifthDCTime, "%4s", msg + 39);
//				sscanf((const char *)s.FifthDPVal, "%2s", msg + 43);
//				msg[45] = 0;
//			}
			
			buf = DataSendToBSN((unsigned char *)"03", "FFFF", (const char *)msg, &size);
			ZigbTaskFreeMemory(msg);
	
			message.type = TYPE_IOT_SEND_DATA;
			message.length = size;
			memcpy(message.infor, buf, size);
				
			xQueueSend(__ZigbeeQueue, &message, configTICK_RATE_HZ); 			
		}
		
		sscanf(p, "%*67s%2s", SyncFlag);
		time_m = atoi((const char *)SyncFlag);
		
		sscanf(p, "%*69s%2s", SyncFlag);
		time_d = atoi((const char *)SyncFlag);
		
		sscanf(p, "%*71s%2s", SyncFlag);
		time_w = atoi((const char *)SyncFlag);
		
		second = RtcGetTime();
		SecondToDateTime(&dateTime, second);
		if((dateTime.month != time_m) || (dateTime.date != time_d) || (dateTime.week != time_w)){    /*镇流器时间对照*/
			
			msg = ZigbtaskApplyMemory(20);
			sprintf((char *)msg, "%02d%02d%02d%02d%02d%02d%02d%02d%02d", dateTime.month, dateTime.date, dateTime.week, 
																								dateTime.hour, dateTime.minute, *DayToNight(), *(DayToNight() + 1),
																								*DayToSunshine(), *(DayToSunshine() + 1));

					
			buf = DataSendToBSN((unsigned char *)"0B", (unsigned char *)"FFFF", (const char *)msg, &size);
			ZigbTaskFreeMemory(msg);

			message.type = TYPE_IOT_SEND_DATA;
			message.length = size;
			memcpy(message.infor, buf, size);
				
			xQueueSend(__ZigbeeQueue, &message, configTICK_RATE_HZ); 		
		}
		
		sscanf(p, "%*11s%2s", space);
		i = atoi((const char *)space);
		
		sscanf(p, "%*21s%4s", SyncFlag);
		if((SyncFlag[0] == '0') && (SyncFlag[1] == '0') && (SyncFlag[2] == '0') && (SyncFlag[3] == '0')){
			compare = 0;
		} else {
			compare = atoi((const char *)SyncFlag);
		}
		
		if((k.CommState != i) || (k.InputPower > (compare + 15)) || ((k.InputPower + 15) < compare)){   /*当前输入功率与上次功率的比较*/  /*当前工作状态与上次的状态比较*/

			msg = ZigbtaskApplyMemory(34 + 9);
			memcpy(msg, "B000", 4);
			memcpy((msg + 4), header->AD, 4);
			memcpy((msg + 4 + 4), (p + 9), 34);
			msg[38 + 4] = 0;
		
			buf = ProtocolRespond(g.GWAddr, (unsigned char *)"06", (const char *)msg, &size);
			GsmTaskSendTcpData((const char *)buf, size);
			
			ZigbTaskFreeMemory(msg);
			
			k.CommState = i;
			k.InputPower = compare;
			
			NorFlashWrite(NORFLASH_BALLAST_BASE + instd * NORFLASH_SECTOR_SIZE, (const short *)&k, (sizeof(Lightparam) + 1) / 2);	
			return;
		}
	}
}

static void ZigbeeHandleBSNUpgrade(FrameHeader *header, unsigned char CheckByte, const char *p){
	
}

extern char HexToAscii(char hex);

typedef void (*ZigbeeHandleFunction)(FrameHeader *header, unsigned char CheckByte, const char *p);
typedef struct {
	unsigned char type[2];
	ZigbeeHandleFunction func;
}ZigbeeHandleMap;

void __handleIOTRecieve(ZigbTaskMsg *p) {
	int i;
	char abNormal = 0;
	FrameHeader h;
	const char *dat = __ZigbGetMsgData(p);
		
	const static ZigbeeHandleMap map[] = {  
		{"02",  ZigbeeHandleLightParam},       /*0x02; 灯参数下载*/             /// 
		{"03",  ZigbeeHandleStrategy},         /*0x03; 策略下载*/               ///
		{"06",  ZigbeeHandleReadBSNData},      /*0x06; 读镇流器数据*/
		{"2A",  ZigbeeHandleBSNUpgrade},       /*0x2A; 镇流器远程升级*/            
	};
	
	h.FH = 0x02;
	sscanf(dat, "%*1s%4s", h.AD);
	sscanf(dat, "%*5s%2s", h.CT);
	sscanf(dat, "%*7s%2s", h.LT);
	if(h.CT[0] > '9'){
		h.CT[0] = h.CT[0] - '7';
	} else {
		h.CT[0] = h.CT[0] - '0';
	}
	h.CT[0] = HexToAscii(h.CT[0] - 8);

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if(strncmp((char *)&(h.CT), (char *)&(map[i].type[0]), 2) == 0){
			map[i].func(&h, abNormal, dat);
			break;
		}	
	}
}

void __handleIOTSend(ZigbTaskMsg *p){
	int i;
	char *dat = __ZigbGetMsgData(p);
	
	for(i = 0; i < p->length; i++){
		SZ05SendChar(*dat++);
	}
	
	vTaskDelay(configTICK_RATE_HZ);
}

static void __handleLampParamDown(ZigbTaskMsg *p){
	
}

static void __handleStrategyDown(ZigbTaskMsg *p){
	
}

static void __HandleLampDimming(ZigbTaskMsg *p){
	
}

static void __handleLampOnAndOff(ZigbTaskMsg *p){
	
}

static void __handleBSNTimming(ZigbTaskMsg *p){
	
}

typedef struct {
	ZigbTaskMsgType type;
	void (*handlerFunc)(ZigbTaskMsg *);
} HandlerOfMap;

static const HandlerOfMap __HandlerOfMaps[] = {
	{ TYPE_IOT_LAMPPARAM_DOWNLOAD, __handleLampParamDown},
	{ TYPE_IOT_STRATEGU_DOWNLOAD,  __handleStrategyDown},
	{ TYPE_IOT_LAMP_DIMMING,       __HandleLampDimming},
	{ TYPE_IOT_LAMP_CONTROL,       __handleLampOnAndOff },
  { TYPE_IOT_TIMING,             __handleBSNTimming},
	{ TYPE_IOT_RECIEVE_DATA,       __handleIOTRecieve},
	{ TYPE_IOT_SEND_DATA,          __handleIOTSend},
	{ TYPE_IOT_NONE, NULL },
};

void ZigbeeHandler(ZigbTaskMsg *p) {
	const HandlerOfMap *map = __HandlerOfMaps;
	for (; map->type != TYPE_IOT_NONE; ++map) {
		if (p->type == map->type) {
			map->handlerFunc(p);
			break;
		}
	}
}

static void ZIGBEETask(void *parameter) {
	portBASE_TYPE rc;
	ZigbTaskMsg message;
	int NumOfAddr = 0;
	short tmp[3];
	unsigned char *buf, ID[5], size; 
	Lightparam k;
	
	for (;;) {
	//	printf("ZIGBEE: loop again\n");
		rc = xQueueReceive(__ZigbeeQueue, &message, configTICK_RATE_HZ / 20);
		if (rc == pdTRUE) {
			ZigbeeHandler(&message);
		} else {
			
			NorFlashRead(NORFLASH_LIGHT_NUMBER, (short *)tmp, 2);
			
			if(tmp[0] == 0)              /*灯参个数*/
				continue;
			
			if(NumOfAddr > tmp[1]){    /*最大ZigBee地址*/
				NumOfAddr = 0;
			}		

			NorFlashRead((NORFLASH_BALLAST_BASE + NumOfAddr * NORFLASH_SECTOR_SIZE), (short *)&k, (sizeof(Lightparam) + 1) / 2);
			
			NumOfAddr++;
			
			if(k.Loop == 0xFF)
				continue;
			
			vTaskDelay(configTICK_RATE_HZ / 10);
			
			sscanf((const char *)k.AddrOfZigbee, "%4s", ID);
			buf = DataSendToBSN("06", ID, NULL, &size);			
			ZigbTaskSendData((const char *)buf, size);
			
		}
	}
}

extern void ProtocolInit(void);

void SHUNCOMInit(void) {
	SZ05_ADV_Init();
	Zibee_Baud_CFG(9600);
	ProtocolInit();
	memset(BSNinfor, 0, 24000);
	__ZigbeeQueue = xQueueCreate(30, sizeof(ZigbTaskMsg));
	xTaskCreate(ZIGBEETask, (signed portCHAR *) "ZIGBEE", ZIGBEE_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}
