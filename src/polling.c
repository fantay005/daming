#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "shuncom.h"
#include "norflash.h"
#include "elegath.h"
#include "second_datetime.h"
#include "rtc.h"

#define POLL_TASK_STACK_SIZE			     (configMINIMAL_STACK_SIZE + 1024 * 5)

#define AUTO_UPDATA_ELEC_PARAM_TIME     (configTICK_RATE_HZ * 30)

#define GSM_GPRS_HEART_BEAT_TIME        (configTICK_RATE_HZ * 60)

extern void *DataFalgQueryAndChange(char Obj, unsigned short Alter, char Query);

extern unsigned char *__datamessage(void);

extern short CallTransfer(void);

extern char *SpaceShift(void);



static void ShortToStr(unsigned short *s, char *r){
	sprintf(r, "%04d", *s);							
}

static short Max_Loop = 0;

typedef enum {
	TYPE_NONE = 0,
	TYPE_GPRS_DATA,         /*网关下发灯参数*/
	TYPE_RTC_DATA,          /*网关下发单灯策略*/
	TYPE_SEND_TCP_DATA,     /*服务器下发查询镇流器数据*/
	TYPE_CSQ_DATA,          /*服务器下发查询电量数据*/
	TYPE_RESET,             /*网关*/
	TYPE_SEND_AT,           /**/
	TYPE_SEND_SMS,          /**/
	TYPE_SETIP,
} PollTaskMessageType;    /*轮询任务类型*/



static void POLLTask(void *parameter) {
	static char MarkRead, OverTurn;
	int len = 0, NumOfAddr= 0;
	Lightparam k;
	FrameHeader h;
	GMSParameter a;
	StrategyParam s;
	unsigned char *buf, ID[16], size, *msg, *alter, *bum;
	char *p;
	unsigned short *ret, tmp[3];
	DateTime dateTime;
	uint32_t second, ResetTime;

	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&a, (sizeof(GMSParameter)  + 1)/ 2);
	
	while(1){		
		
		WatchdogFeed();
		
		vTaskDelay(configTICK_RATE_HZ / 5);	

		bum = DataFalgQueryAndChange(5, 0, 1);
	//	printf("Hello");
		if(*bum){
			unsigned char *command;
			short Numb;
			
			command = DataFalgQueryAndChange(2, 0, 1);
			
			switch(*command){
					
				case 2:		

					buf = DataSendToBSN((unsigned char *)"04", (unsigned char *)"FFFF", (const char *) __datamessage(), &size);
					ZigbTaskSendData((const char *)buf, size);
					vPortFree(buf);
				
					ret = DataFalgQueryAndChange(1, 0, 1);
					while(*ret){		
						ShortToStr(ret, (char *)ID);						
						alter = DataSendToBSN((unsigned char *)"06", ID, NULL, &size);
						DataFalgQueryAndChange(4, 1, 0);
						ZigbTaskSendData((const char *)alter, size);
						ret++;
						vTaskDelay(configTICK_RATE_HZ / 5);	
					}
					break;
				
				case 3:		
					buf = DataSendToBSN((unsigned char *)"05", (unsigned char *)"FFFF", (const char *) __datamessage(), &size);
					ZigbTaskSendData((const char *)buf, size);
				
					ret = DataFalgQueryAndChange(1, 0, 1);
					while(*ret){
						ShortToStr(ret, (char *)ID);
						alter = DataSendToBSN((unsigned char *)"06", ID, NULL, &size);
						DataFalgQueryAndChange(4, 1, 0);
						ZigbTaskSendData((const char *)alter, size);
						ret++;
						vTaskDelay(configTICK_RATE_HZ / 5);	
					}						
					break;
				
				case 5:
					Numb = CallTransfer();
					NorFlashRead(NORFLASH_BALLAST_BASE + Numb * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
					msg = pvPortMalloc(26 + 1);
			
					sscanf((const char *)k.TimeOfSYNC, "%12s", msg);
					sscanf((const char *)k.NorminalPower, "%4s", msg + 12);
					msg[16] = k.Loop;
					sscanf((const char *)k.LightPole, "%4s", msg + 17);
					msg[21] = k.LightSourceType;
					msg[22] = k.LoadPhaseLine;
					sscanf((const char *)k.Attribute, "%2s", msg + 23);
					msg[25] = 0;
				
					#if defined(__HEXADDRESS__)
							sprintf((char *)h.AD, "%04X", Numb);
					#else				
							sprintf((char *)h.AD, "%04d", Numb);
					#endif	
				
					buf = DataSendToBSN((unsigned char *)"02", h.AD, (const char *)msg, &size);
					ZigbTaskSendData((const char *)buf, size);				
					break;
					
				case 6:                               /*网关下发策略*/
//					Numb = CallTransfer();
					NorFlashRead(NORFLASH_STRATEGY_ADDR, (short *)&s, (sizeof(StrategyParam) + 1) / 2);
					msg = pvPortMalloc(47 + 1);
					
					sscanf((const char *)s.SYNCTINE, "%12s", msg);
					sscanf((const char *)s.SchemeType, "%2s", msg + 12);
					msg[14] = s.DimmingNOS;
				
					sscanf((const char *)s.FirstDCTime, "%4s", msg + 15);
					sscanf((const char *)s.FirstDPVal, "%2s", msg + 19);
					msg[21] = 0;
				
					if ((s.DimmingNOS - '0') == 2){
						sscanf((const char *)s.SecondDCTime, "%4s", msg + 21);
						sscanf((const char *)s.SecondDPVal, "%2s", msg + 25);	
						msg[27] = 0;
					}
					
					if ((s.DimmingNOS - '0') == 3){
						sscanf((const char *)s.ThirdDCTime, "%4s", msg + 27);
						sscanf((const char *)s.ThirdDPVal, "%2s", msg + 31);
						msg[33] = 0;
					}
					
					if ((s.DimmingNOS - '0') == 4){
						sscanf((const char *)s.FourthDCTime, "%4s", msg + 33);
						sscanf((const char *)s.FourthDPVal, "%2s", msg + 37);
						msg[39] = 0;
					}
					
					if ((s.DimmingNOS - '0') == 5){
						sscanf((const char *)s.FifthDCTime, "%4s", msg + 39);
						sscanf((const char *)s.FifthDPVal, "%2s", msg + 43);
						msg[45] = 0;
					}
					
					buf = DataSendToBSN((unsigned char *)"03", "FFFF", (const char *)msg, &size);
					ZigbTaskSendData((const char *)buf, size);					
					break;
					
				case 7:	                                   /*网关下发当前时间和开关灯时间*/
					
				default:
					break;	
				
			}
			DataFalgQueryAndChange(3, 0, 0);
			DataFalgQueryAndChange(2, 0, 0);
			DataFalgQueryAndChange(5, 0, 0);
		} 
	}
}

void POLLSTART(void) {
	
	xTaskCreate(POLLTask, (signed portCHAR *) "POLL", POLL_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
}

