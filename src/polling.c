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

extern unsigned char *ProtocolToElec(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);
extern unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size);
extern void *DataFalgQueryAndChange(char Obj, unsigned short Alter, char Query);
extern unsigned short PowerStatus(void);

static bool JudgeParam(unsigned char param){
	if(param < '0'){
		return true;
	}
	
	if(param > 'F'){
		return true;
	}
	
	if((param > '9') && (param < 'A')){
		return true;
	}
	
	return false;
}

extern char SendStatus(void);

extern unsigned char *__datamessage(void);

extern short CallTransfer(void);

extern char *SpaceShift(void);

extern char Conect_server_start(void);

extern bool GSMTaskSendErrorTcpData(void);

static void ShortToStr(unsigned short *s, char *r){

	sprintf(r, "%04d", *s);							
}

static short MAX = 0;

static short Max_Loop = 0;

//static unsigned int TotalOfPower = 0;

//static char ELEC_FLAG = 0;

static void POLLTask(void *parameter) {
	static char MarkRead, count = 0, OverTurn;
	int len = 0, i = 1, NumOfAddr= 0;
//	unsigned int sum;
	Lightparam k;
	FrameHeader h;
	GMSParameter a;
	unsigned char *buf, ID[16], size, *msg, *alter, *bum;
	char *p;
	portTickType HeartT = 0, UpdataTime = 0;
	unsigned short *ret, tmp[3];
	DateTime dateTime;
	uint32_t second, ResetTime;
//unsigned char *addr = (unsigned char *)"FFFF";

	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&a, (sizeof(GMSParameter)  + 1)/ 2);
	
	while(1){	
		
		bum = DataFalgQueryAndChange(5, 0, 1);
	//	printf("Hello");
		if(*bum){
			unsigned char *command;
			short Numb;
			
			command = DataFalgQueryAndChange(2, 0, 1);
			
			switch(*command){
				case 1:
					ret = DataFalgQueryAndChange(1, 0, 1);
					while(*ret){
						ShortToStr(ret, (char *)ID);
						alter = DataSendToBSN((unsigned char *)"06", ID, NULL, &size);
						DataFalgQueryAndChange(4, 1, 0);
						ZigbTaskSendData((const char *)alter, size);
						vPortFree(alter);
						
						ret++;
					//	vTaskDelay(configTICK_RATE_HZ / 5);	
					}	
					break;
					
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
						vPortFree(alter);
						ret++;
						vTaskDelay(configTICK_RATE_HZ / 5);	
					}
					break;
				
				case 3:		
					buf = DataSendToBSN((unsigned char *)"05", (unsigned char *)"FFFF", (const char *) __datamessage(), &size);
					ZigbTaskSendData((const char *)buf, size);
					vPortFree(buf);
				
					ret = DataFalgQueryAndChange(1, 0, 1);
					while(*ret){
						ShortToStr(ret, (char *)ID);
						alter = DataSendToBSN((unsigned char *)"06", ID, NULL, &size);
						DataFalgQueryAndChange(4, 1, 0);
						ZigbTaskSendData((const char *)alter, size);
						vPortFree(alter);		
						ret++;
						vTaskDelay(configTICK_RATE_HZ / 5);	
					}						
					break;
				
				case 4:
					bum = DataFalgQueryAndChange(6, 0, 1);
					ID[0] = *bum;
					ID[1] = 0;
				
					//vTaskDelay(configTICK_RATE_HZ * 5);
				
					if(ID[0] != '0'){
						buf = ProtocolToElec(a.GWAddr, (unsigned char *)"08", (const char *)ID, &size);
						ElecTaskSendData((const char *)buf, size);	
						vPortFree(buf);	
					} else {
						if(Max_Loop == 0){
							break;
						}

						sprintf((char *)ID, "%02d", 0);
						
						buf = ProtocolToElec(a.GWAddr, (unsigned char *)"08", (const char *)ID, &size);
						ElecTaskSendData((const char *)buf, size); 
						vPortFree(buf);
						
					}
					//vTaskDelay(configTICK_RATE_HZ * 5);
					
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
					vPortFree(buf);
					vPortFree(msg);				
					break;	
					
				case 7:	

					break;
				
				case 8:			
				  p = SpaceShift();
					Numb = CallTransfer();
				
					#if defined(__HEXADDRESS__)
							sprintf((char *)h.AD, "%04X", Numb);
					#else				
							sprintf((char *)h.AD, "%04d", Numb);
					#endif	
		
					msg = pvPortMalloc(34 + 9);
					memcpy(msg, "B000", 4);
					memcpy((msg + 4), h.AD, 4);
					memcpy((msg + 4 + 4), (p + 9), 34);
					msg[38 + 4] = 0;
				
					vPortFree(p);
					buf = ProtocolRespond(a.GWAddr, (unsigned char *)"06", (const char *)msg, &size);
					GsmTaskSendTcpData((const char *)buf, size);
					vPortFree(buf);
					vPortFree(msg);
					break;
					
				default:
					break;	
				
			}
			DataFalgQueryAndChange(3, 0, 0);
			DataFalgQueryAndChange(2, 0, 0);
			DataFalgQueryAndChange(5, 0, 0);
		} else {
			short MaxAddr;
//			portTickType curT;	
//			curT = xTaskGetTickCount();
//			
//			if ((curT - UpdataTime) >= AUTO_UPDATA_ELEC_PARAM_TIME) {
//					buf = ProtocolToElec(a.GWAddr, (unsigned char *)"08", (const char *)"0", &size);
//					ElecTaskSendData((const char *)buf, size); 
//					vPortFree(buf);
//					
//					UpdataTime = curT;
//					continue;
//			  } 
			
			if(NumOfAddr >= (MAX + 50)){
				short ret[3] = {0};
				
				ret[0] = MAX;
				ret[1] = MaxAddr;
				NorFlashRead(NORFLASH_LIGHT_NUMBER, (short *)tmp, 2);
				if(((MAX != tmp[0]) && (MAX < 1000)) || (MaxAddr != tmp[1])){
					NorFlashWrite(NORFLASH_LIGHT_NUMBER, (short *)ret, 2);
				} else if(tmp[0] == 0xFFFF){
					NorFlashWrite(NORFLASH_LIGHT_NUMBER, (short *)ret, 2);
				}
					
				NumOfAddr = 1;
				MAX = 0;
			}		

			NorFlashRead((NORFLASH_BALLAST_BASE + NumOfAddr * NORFLASH_SECTOR_SIZE), (short *)&k, (sizeof(Lightparam) + 1) / 2);
			
#if defined(__MODEL_DEBUG__)	
			
#else				
//			if((k.InputPower > 0) && (k.InputPower < 1000)){
//				sum += k.InputPower; 
//				if(NumOfAddr == 0){
//					TotalOfPower = sum;
//					sum = 0;
//					ELEC_FLAG++;
//				}	
//			}
//			
//			if((ELEC_FLAG == 2) && (TotalOfPower > PowerStatus())){
//				vTaskDelay(configTICK_RATE_HZ * 5);
//				for(i = 0; i < (Max_Loop + 1); i++){						
//					sprintf((char *)ID, "%d", i);
//					*(ID + 1) = 0;
//					bum = ProtocolToElec(a.GWAddr, (unsigned char *)"08", (const char *)ID, &size);
//					ElecTaskSendData((const char *)bum, size); 
//					vPortFree(bum);
//				}

//				vTaskDelay(configTICK_RATE_HZ * 2);
//			}
			
//			if(ELEC_FLAG == 2){
//				ELEC_FLAG = 0;
//			}
#endif			
			
			NumOfAddr++;
			if(JudgeParam(k.Loop)){
				continue;
			} else {			
				GatewayParam1 param;				
				
				if(NumOfAddr > MaxAddr)
					MaxAddr = NumOfAddr - 1;            /*取最大地址数*/
				if((k.Loop - '0') > Max_Loop){
					Max_Loop = k.Loop - '0';
				}
					
				NorFlashRead(NORFLASH_MANAGEM_BASE, (short *)&param, (sizeof(GatewayParam1) + 1) / 2);
				
				if(MarkRead == 0){			
					MarkRead = 1;
				}
				if(JudgeParam(param.IntervalTime[0])){
					continue;
				}
				if(JudgeParam(param.IntervalTime[1])){
					continue;
				}
				
				if(MarkRead == 1){
					if((param.IntervalTime[0] >= '0') && (param.IntervalTime[0] <= '9')){
						len = ((param.IntervalTime[0] << 4) & 0xF0);
					} else {
						len = (((param.IntervalTime[0] - '7') << 4) & 0xF0);
					}
					
					if((param.IntervalTime[1] >= '0') && (param.IntervalTime[1] <= '9')){
						len |= ((param.IntervalTime[1]) & 0x0F);
					} else {
						len |= ((param.IntervalTime[1] - '7') & 0x0F);
					}
					MarkRead = 2;
				}		
				
				second = RtcGetTime();
				SecondToDateTime(&dateTime, second);
				
				if(dateTime.minute > 5){
					MarkRead = 3;
				}
						
				if((MarkRead == 2) || ((dateTime.minute < 8) && (OverTurn == 0))){
					
					if(Max_Loop == 0){
						continue;
					}
				
					NorFlashRead(NORFLASH_ELEC_UPDATA_TIME, (short *)tmp, 2);
					ResetTime = (tmp[0] << 16) | tmp[1];
					
				//	vTaskDelay(configTICK_RATE_HZ * 5);
					sprintf((char *)ID, "%02d", 0);
					
					if((second - ResetTime) > 60){
						buf = ProtocolToElec(a.GWAddr, (unsigned char *)"08", (const char *)ID, &size);
						ElecTaskSendData((const char *)buf, size); 
						vPortFree(buf);
					}
					
				//	vTaskDelay(configTICK_RATE_HZ * 5);
					
					if(MarkRead == 2){
						tmp[0] = second >> 16;
						tmp[1] = second & 0xFFFF;
						tmp[2] = 0;
						NorFlashWrite(NORFLASH_RESET_TIME, (short *)tmp, 2);
					}
					
					OverTurn = 1;
					MarkRead = 4;
		
				} 
				
				if(dateTime.minute >= 8) {
					OverTurn = 0;
				}			
				
				MAX++;
				
				sscanf((const char *)k.AddrOfZigbee, "%4s", ID);
				h.CT[0] = '0';
				h.CT[1] = '6';
				buf = DataSendToBSN(h.CT, ID, NULL, &size);			
				ZigbTaskSendData((const char *)buf, size);
				count++;
				vPortFree(buf);	
			}
		}
	}
}

void POLLSTART(void) {
	
	xTaskCreate(POLLTask, (signed portCHAR *) "POLL", POLL_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}

