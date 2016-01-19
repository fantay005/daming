#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "norflash.h"
#include "zklib.h"
#include "libupdater.h"
#include "norflash.h"
#include "second_datetime.h"
#include "rtc.h"
#include "version.h"
#include "elegath.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_flash.h"
#include "shuncom.h"
#include "time.h"
#include "semphr.h"
#include "inside_flash.h"

static xSemaphoreHandle __ProSemaphore;

typedef enum{
	ACKERROR = 0,           /*��վӦ���쳣*/
	GATEPARAM = 0x01,       /*���ز�������*/
	LIGHTPARAM = 0x02,      /*�Ʋ�������*/
	DIMMING = 0x04,         /*�Ƶ������*/
	LAMPSWITCH = 0x05,      /*�ƿ��ؿ���*/
	READDATA = 0x06,        /*������������*/
	LOOPCONTROL = 0x07,     /*���ػ�·����*/
	DATAQUERY = 0x08,       /*�������ݲ�ѯ*/
	VERSIONQUERY = 0x0C,    /*��������汾�Ų�ѯ*/ 
  ELECTRICGATHER = 0x0E,  /*�����ɼ�����汾�Ų�ѯ*/	
	GATEUPGRADE = 0x37,     /*����Զ������*/
	TIMEADJUST = 0x42,      /*Уʱ*/
	LUXVALUE = 0x43,        /*���յ���ǿ��ֵ*/
	RESTART = 0x3F,         /*�豸��λ*/
	RETAIN,                 /*����*/
} GatewayType;

typedef struct {
	unsigned char BCC[2];
	unsigned char x03;
} ProtocolTail;


unsigned char *ProtocolMessage(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	char HexToAscii[] = {"0123456789ABCDEF"};

	*size = 15 + len + 3;
	if(type[1] > '9'){
		i = (unsigned char)(type[0] << 4) | (type[1] - 'A' + 10);
	} else{
		i = (unsigned char)(type[0] << 4) | (type[1] & 0x0f);
	}
	
	ret = pvPortMalloc(*size + 1);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;	
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = HexToAscii[i >> 4];
		h->contr[1] = HexToAscii[i & 0x0F];
		h->lenth[0] = HexToAscii[(len >> 4) & 0x0F];
		h->lenth[1] = HexToAscii[len & 0x0F];
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 15), msg);
	}
	
	p = ret;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = HexToAscii[(verify >> 4) & 0x0F];
	*p++ = HexToAscii[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
	return ret;
}

unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char hexTable[] = "0123456789ABCDEF";
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 9 + len + 3 + 2;
	
	ret = pvPortMalloc(*size + 1);
	
	
	if(strncmp((const char *)address, "FFFF", 4) == 0){
		*ret = 0xFF;
		*(ret + 1) = 0xFF;
	} else {
		*ret = (address[0] << 4) | (address[1] & 0x0f);
		*(ret + 1) = (address[2] << 4) | (address[3] & 0x0f);

	}
	{
		FrameHeader *h = (FrameHeader *)(ret + 2);
		h->FH = 0x02;	
		strcpy((char *)&(h->AD[0]), (const char *)address);
		h->CT[0] = control[0];
		h->CT[1] = control[1];
		h->LT[0] = hexTable[(len >> 4) & 0x0F];
		h->LT[1] = hexTable[len & 0x0F];
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 11), msg);
	}
	
	p = ret + 2;
	
	verify = 0;
	for (i = 0; i < (len + 9); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
	return ret;
}

extern char HexToAscii(char hex);

unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 15 + len + 3;
	if(type[1] > '9'){
		i = (unsigned char)(type[0] << 4) | (type[1] - 'A' + 10);
	} else{
		i = (unsigned char)(type[0] << 4) | (type[1] & 0x0f);
	}
	i = i | 0x80;
	ret = pvPortMalloc(*size + 1);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;	
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = HexToAscii(i >> 4);
		h->contr[1] = HexToAscii(i & 0x0F);
		h->lenth[0] = HexToAscii((len >> 4) & 0x0F);
		h->lenth[1] = HexToAscii(len & 0x0F);
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 15), msg);
	}
	
	p = ret;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = HexToAscii((verify >> 4) & 0x0F);
	*p++ = HexToAscii(verify & 0x0F);
	*p++ = 0x03;
	*p = 0;
	return ret;
}

unsigned char *ProtocolToElec(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	int i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	
	*size = 15 + len + 3;
	i = (unsigned char)(type[0] << 4) + (type[1] & 0x0f);
	
	ret = pvPortMalloc(*size + 1);
	{
		ProtocolHead *h = (ProtocolHead *)ret;
		h->header = 0x02;
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = HexToAscii(i >> 4);
		h->contr[1] = HexToAscii(i & 0x0F);
		h->lenth[0] = HexToAscii((len >> 4) & 0x0F);
		h->lenth[1] = HexToAscii(len & 0x0F);
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 15), msg);
	}
	
	p = ret;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = HexToAscii((verify >> 4) & 0x0F);
	*p++ = HexToAscii(verify & 0x0F);
	*p++ = 0x03;
	*p = 0;
	return ret;
}


void ProtocolDestroyMessage(const char *p) {
	vPortFree((void *)p);
}

static void HandleGatewayParam(ProtocolHead *head, const char *p) {
	unsigned char size;
	unsigned char *buf, msg[2];

	if(strlen(p) == (50 - 15)){
		GatewayParam1 g;
		sscanf(p, "%*1s%6s", g.GatewayID);
		sscanf(p, "%*7s%10s", g.Longitude);
		sscanf(p, "%*17s%10s", g.Latitude);
	//	sscanf(p, "%*13s%1s", g->FrequPoint);
		g.FrequPoint = p[27];
		sscanf(p, "%*28s%2s", g.IntervalTime);
		sscanf(p, "%*30s%2s", g.Ratio);
		g.EmbedInformation = 1;
		NorFlashWrite(NORFLASH_MANAGEM_BASE, (const short *)&g, (sizeof(GatewayParam1) + 1) / 2);
	}  else if(strlen(p) == (78 - 15)){
		GatewayParam3 g;
		sscanf(p, "%*1s%4s", g.HVolLimitValL1);
		sscanf(p, "%*5s%4s", g.HVolLimitValL2);
		sscanf(p, "%*9s%4s", g.HVolLimitValL3);
		
		sscanf(p, "%*13s%4s", g.LVolLimitValL1);
		sscanf(p, "%*17s%4s", g.LVolLimitValL2);
		sscanf(p, "%*21s%4s", g.LVolLimitValL3);

		sscanf(p, "%*25s%4s", g.NoloadCurLimitValL1);
		sscanf(p, "%*29s%4s", g.NoloadCurLimitValL2);
		sscanf(p, "%*33s%4s", g.NoloadCurLimitValL3);
		sscanf(p, "%*37s%4s", g.NoloadCurLimitValN);
		
		sscanf(p, "%*41s%4s", g.PhaseCurLimitValL1);
		sscanf(p, "%*45s%4s", g.PhaseCurLimitValL2);
		sscanf(p, "%*49s%4s", g.PhaseCurLimitValL3);
		sscanf(p, "%*53s%4s", g.PhaseCurLimitValN);
//		sscanf(p, "%*16s%2s", g->NumbOfCNBL);
		g.NumbOfCNBL = p[57];
		sscanf(p, "%*58s%2s", g.OtherWarn);
		g.SetWarnFlag = 1;
//		NorFlashWriteChar(NORFLASH_MANAGEM_WARNING, (const char *)g, sizeof(GatewayParam3));
		NorFlashWrite(NORFLASH_MANAGEM_WARNING, (const short *)&g, (sizeof(GatewayParam3) + 1) / 2);
	}
	
	sprintf((char *)msg, "%c", p[0]);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);
}

static void __ParamWriteToFlash(const char *p){
	unsigned char msg[5];
	unsigned short len;
	DateTime dateTime;
	uint32_t second;	
	Lightparam g;
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);
	
	sscanf(p, "%4s", g.AddrOfZigbee);

		sscanf(p, "%*4s%4s", g.NorminalPower);
//		sscanf(p, "%*16s%12s", g->Loop);
		g.Loop = p[8];
		
		sscanf(p, "%*9s%4s", g.LightPole);		
		len = atoi((const char *)g.LightPole);
		
		if(len >= 9000){
			
		}
//		sscanf(p, "%*16s%12s", g->LightSourceType);
		g.LightSourceType = p[13];
//		sscanf(p, "%*16s%12s", g->LoadPhaseLine);
		g.LoadPhaseLine = p[14];
		sscanf(p, "%*15s%2s", g.Attribute);
		
		sprintf((char *)g.TimeOfSYNC, "%02d%02d%02d%02d%02d%02d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
 
		g.CommState = 0x04;
		g.InputPower = 0;
		
	//	NorFlashWriteChar(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const char *)g, sizeof(Lightparam));
		
		sscanf(p, "%4s",msg);
	
	  len = atoi((const char *)msg);	
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(Lightparam) + 1) / 2);	
}

static void HandleLightParam(ProtocolHead *head, const char *p) {
	int len, i, lenth;
	unsigned char size;
	DateTime dateTime;
	uint32_t second;	
	unsigned char *buf, msg[48], tmp[5];
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);

	if(p[0] == '1'){          /*����һյ��*/
		
		len = (strlen(p) - 1) / 17;

		for(i = 0; i < len; i++) {
			__ParamWriteToFlash(&p[1 + i * 17]);
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4])); 
		}
		msg[i * 4] = p[0];
		
	} else if (p[0] == '2'){   /*ɾ��һյ��*/
		len = (strlen(p) - 18) / 17;

		for(i = 0; i < len; i++) {
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4]));
			sscanf(&p[1 + i * 17], "%4s", tmp);
		
			lenth = atoi((const char *)tmp);
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + lenth * NORFLASH_SECTOR_SIZE);
		}
		msg[i * 4] = p[0];
		
	} else if (p[0] == '4'){
		for(len = 0; len < 1000; len++){
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		}
	}
	
	msg[i * 4 + 1] = 0;
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

typedef struct{	
	unsigned short ArrayAddr[600];
	unsigned char SendFlag;
	unsigned char ProtectFlag;
	unsigned char Command;
	unsigned char NoReply;
	unsigned char NumberOfLoop;
	unsigned short Lenth;
	unsigned char Answer;
	unsigned char Sort;
}ReadBSNData;

static ReadBSNData __msg = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void ProtocolInit(void) {
	memset(__msg.ArrayAddr, 0, 600);
	if (__ProSemaphore == NULL) {
		vSemaphoreCreateBinary(__ProSemaphore);
	}
}

void *DataFalgQueryAndChange(char Obj, unsigned short Alter, char Query){
	if (xSemaphoreTake(__ProSemaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		if(Query == 0){
			switch (Obj){
				case 1:
					__msg.ArrayAddr[(__msg.Sort)++] = Alter;
					break;
				case 2:
					__msg.Command = Alter;
					break;
				case 3:
					__msg.NoReply = Alter;
					break;
				case 4:
					__msg.SendFlag = Alter;
					break;
				case 5:
					__msg.ProtectFlag = Alter;
					break;
				case 6:
					__msg.NumberOfLoop = Alter;
					break;
				case 7:
					__msg.Lenth = Alter;
				case 8:
					__msg.Sort = 0;
					break;
				default:
					break;
			}
		} 
		xSemaphoreGive(__ProSemaphore);
		if(Query == 1){
			switch (Obj){
				case 1:
					return __msg.ArrayAddr;
				case 2:
					return &(__msg.Command);
				case 3:
					return &(__msg.NoReply);
				case 4:
					return &(__msg.SendFlag);
				case 5:
					return &(__msg.ProtectFlag);
				case 6:
					return &(__msg.NumberOfLoop);
				case 7:
					return &(__msg.Lenth);
				case 8:
					return &(__msg.Answer);
				default:
					break;
			}
		}
	}
	return false;
}

typedef enum{
	ENTRANCE = 1,       /*��ڶ�*/
	TRANSITION_I,       /*��ڹ��ɶ�*/
	INTERMEDIATE,       /*�м��*/
	TRANSITON_II,       /*���ڹ��ɶ�*/
	EXITSECTION,        /*���ڶ�*/
	GUIDESECTION,       /*������*/
}PartType;            /*������ζ���*/

#define NoChoice     0           /*����û��ѡ��*/

void SeekAddress(PartType loop, unsigned short pole, char main, char aux){     /*���ݵƲβ���zigbee��ַ*/
	int i = 0, j, len = 0, m, n;
	unsigned char tmp[5], *ret, *msg;
	Lightparam k;
	
	msg = DataFalgQueryAndChange(2, 0, 1);

	memset(__msg.ArrayAddr, 0, 600);
	for(len = 0; len < 599; len++) {
		NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
		
		if(k.Loop != (loop + '0')){            /*��·Ҫһ��*/
			continue;
		}
		
		sscanf((const char *)k.LightPole, "%4s", tmp);
		m = atoi((const char *)tmp);           /*�Ƹ˺�*/
		
		if((pole != NoChoice) && (m != pole)){
			continue;
		}
		
		sscanf((const char *)k.Attribute, "%2s", tmp);
		m = atoi((const char *)tmp);           /*����������*/
		
		if(m < 0x10){                          /*������*/
			
			if((main != NoChoice) && (main != (m & 0x0F)))
				continue;
		} else if((m > 0x10) && (m < 0x20)){   /*������*/
			
			if((aux != NoChoice) && (aux != (m & 0x0F)))
				continue;
		} else if(m > 0x30){                   /*Ͷ���*/
			continue;
		}
		
		sprintf((char *)k.AddrOfZigbee, "%4s", tmp);
		__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
	}
	
	__msg.ArrayAddr[i] = 0;
	__msg.Lenth = i;
} 



void __DataFlagJudge(const char *p){
	int i = 0, j, len = 0, m, n;
	unsigned char tmp[5], *ret, *msg;
	Lightparam k;
	
	msg = DataFalgQueryAndChange(2, 0, 1);
	if(*msg == 1){
		ret = (unsigned char *)p + 4;
	} else if(*msg == 3) {
		ret = (unsigned char *)p + 5;
	} else if(*msg == 2) {
		ret = (unsigned char *)p + 6;
	}

	if(p[0] == 'A'){
		memset(__msg.ArrayAddr, 0, 600);
		for(len = 0; len < 599; len++) {
			NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if(k.AddrOfZigbee[0] != 0xff){
				sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
				__msg.ArrayAddr[i++] = atoi((const char *)tmp);
			}
		}
	} else if(p[0] == '8'){
		memset(__msg.ArrayAddr, 0, 600);
		for(len = 0; len < 599; len++) {
			NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if(k.Loop == 0xFF){
				continue;
			}
			
			for(j = 0; j < (p[1] - '0'); j++){
				if(k.Loop != *(ret + j)){
					continue;
				}
				
				if(k.Attribute[0] == '0'){                               /*������*/
					for(m = 0; m < (p[2] - '0'); m++){
						if(k.Attribute[1] == *(ret + p[1] - '0' + m)){
							sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
			
							break;
						}
					}
				} else if(k.Attribute[0] == '1'){	                      /*������*/
					for(n = 0; n < (p[3] - '0'); n++){
						if(k.Attribute[1] == *(ret + p[1] - '0'  + p[2] - '0' + n)){
							sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
							
							break;
						}
					}
				} else {                                               /*Ͷ���*/
						sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
					
							#if defined(__HEXADDRESS__)						
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
					
					  break;
				}
				j = p[1] - '0';				
			}
		}
	} else if(p[0] == '9'){
		memset(__msg.ArrayAddr, 0, 600);
		for(len = 0; len < 599; len++) {
			NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
		  if(k.Loop == 0xFF){
				continue;
			}
			
			for(j = 0; j < (p[1] - '0'); j++){
				if(k.Loop != *(ret + j)){
					continue;
				}
				
				if(k.Attribute[0] == '0'){                           /*������*/
					for(m = 0; m < (p[2] - '0'); m++){
						if(k.Attribute[1] == *(ret + p[1] - '0' + m)){
							sscanf((const char *)k.AddrOfZigbee, "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
							
							break;
						}
					}
				} else if(k.Attribute[0] == '1'){	                   /*������*/
					for(n = 0; n < (p[3] - '0'); n++){
						if(k.Attribute[1] == *(ret + p[1] - '0'  + p[2] - '0' + n)){
							sscanf((const char *)(k.AddrOfZigbee), "%4s", tmp);
							
							#if defined(__HEXADDRESS__)
									__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
							#else		
									__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
							#endif		
							
							break;
						}
					} 
				}
				j = p[1] - '0';		
			}
		}			
	} else if(p[0] == 'B'){
		  memset(__msg.ArrayAddr, 0, 600);
			sscanf((const char *)ret, "%4s", tmp);
		
			#if defined(__HEXADDRESS__)
					__msg.ArrayAddr[i++] = strtol((const char *)tmp, NULL, 16);
			#else		
					__msg.ArrayAddr[i++] = atoi((const char *)tmp);							
			#endif		
		
	}
	__msg.ArrayAddr[i] = 0;
	__msg.Lenth = i;
} 

static unsigned char DataMessage[32];

unsigned char *__datamessage(void){
	return DataMessage;
}

static void HandleLightDimmer(ProtocolHead *head, const char *p){
	unsigned char *buf, msg[8], *ret, size;
	
	ret = DataFalgQueryAndChange(5, 0, 1);
	while(*ret != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	DataFalgQueryAndChange(2, 2, 0);
	sscanf(p, "%6s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	ret = pvPortMalloc(strlen(p) + 1);
	strcpy((char *)ret, p);
	ret[strlen(p) - 3] = 0;
	
	strcpy((char *)DataMessage, (const char *)ret);
	vPortFree(ret);
	
	DataFalgQueryAndChange(3, 1, 0);
	__DataFlagJudge(p);
}

static void HandleLightOnOff(ProtocolHead *head, const char *p) {
	unsigned char msg[8];
	unsigned char *buf, *ret, size;
	
	ret = DataFalgQueryAndChange(5, 0, 1);
	while(*ret != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	DataFalgQueryAndChange(2, 3, 0);
	sscanf(p, "%5s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	ret = pvPortMalloc(strlen(p) + 1);
	strcpy((char *)ret, p);
	ret[strlen(p) - 3] = 0;
	
	strcpy((char *)DataMessage, (const char *)ret);
	vPortFree(ret);
	
	DataFalgQueryAndChange(3, 1, 0);
	__DataFlagJudge(p);
}

static void HandleReadBSNData(ProtocolHead *head, const char *p) {
	unsigned char *buf, msg[8], size;	
	
	buf = DataFalgQueryAndChange(5, 0, 1);
	while(*buf != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	DataFalgQueryAndChange(2, 1, 0);
	sscanf(p, "%4s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	__DataFlagJudge(p);
}

static void HandleGWDataQuery(ProtocolHead *head, const char *p) {     /*���ػ�·���ݲ�ѯ*/
	char *buf;
	
	buf = DataFalgQueryAndChange(5, 0, 1);
	while(*buf != 0){
		vTaskDelay(configTICK_RATE_HZ / 500);
	}
	
	DataFalgQueryAndChange(2, 4, 0);
	DataFalgQueryAndChange(6, p[0], 0);
 // ElecTaskSendData((const char *)(p - sizeof(ProtocolHead)), (sizeof(ProtocolHead) + strlen(p)));

	DataFalgQueryAndChange(5, 1, 0);
}

static void HandleLightAuto(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleAdjustTime(ProtocolHead *head, const char *p) {    /*Уʱ*/
	DateTime ServTime;
	unsigned char *buf, size;

	ServTime.year = (p[0] - '0') * 10 + p[1] - '0';
	ServTime.month = (p[2] - '0') * 10 + p[3] - '0';
	ServTime.date = (p[4] - '0') * 10 + p[5] - '0';
	ServTime.hour = (p[6] - '0') * 10 + p[7] - '0';
	ServTime.minute = (p[8] - '0') * 10 + p[9] - '0';
	ServTime.second = (p[10] - '0') * 10 + p[11] - '0';
	
	RtcSetTime(DateTimeToSecond(&ServTime));
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleGWVersQuery(ProtocolHead *head, const char *p) {      /*����������汾��*/
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, Version(), &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
}

static void HandleEGVersQuery(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond((unsigned char *)"9999999999", head->contr, NULL, &size);
	ElecTaskSendData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleGWUpgrade(ProtocolHead *head, const char *p) {             //FTPԶ������
	FirmwareUpdaterMark *mark;
	char size[6];
	int len, type;
	
	sscanf(p, "%2s", size);
	type = strtol((const char *)size, NULL, 16);
	
	sscanf(&p[2], "%6s", size);
	len = atoi(size);
	mark = pvPortMalloc(sizeof(*mark));
	if (mark == NULL) {
		return;
	}
	
	if (FirmwareUpdateSetMark(mark, len, type)) {
		NVIC_SystemReset();
	}
	vPortFree(mark);
}


static void HandleEGUpgrade(ProtocolHead *head, const char *p) {
	
}

#define BALLAST_UPGRADE_AREA    (0x0805B000)     //  300K + 64K 

static char Ballast_Upgrade_Flag[320] = {0};
static char Ballast_Count = 0;

static void HandleBSNUpgrade(ProtocolHead *head, const char *p) {
	char msg[10];
	int Total, Segment, i;
	unsigned char size, *buf;
	
	sscanf(p, "%4s", msg);
	Total = strtol(msg, NULL, 16);
	
	if(Total > 320){
		return;
	}
	
	sscanf(p, "%*4s%4s", msg);
	Segment = strtol(msg, NULL, 16);
	
	if(Segment > Total){
		return;
	}
	
	Ballast_Upgrade_Flag[Segment - 1] = 1;
	
	STMFLASH_Write((BALLAST_UPGRADE_AREA + (Segment - 1) * 200), (uint16_t *)p, (uint16_t)head->lenth);
	
	sscanf(p, "%8s", msg);
	msg[8] = '0';
	msg[9] = 0;
	
	buf = ProtocolRespond(head->addr, head->contr, msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	
	for(i = 0; i < Total; i++){
		if(Ballast_Upgrade_Flag[i] == 1){
			Ballast_Count++;
		}
	}
	
	if(Ballast_Count == Total){
		DataFalgQueryAndChange(2, 9, 0);
	}
}

void GuideHandleDayLux(int lux){                 /*�����δ��������նȺ���*/

}

void GuideHandleNightLux(int lux){               /*�����δ���ҹ����նȺ���*/
	
}

void GuideHandleLux(char time, int lux){         /*�����δ���ʱ��͹��նȺ���*/
	if(time){
		GuideHandleDayLux(lux);
	} else {
		GuideHandleNightLux(lux);
	}	
}

void MiddleHandleDayLux(int lux){                /*�м�δ��������նȺ���*/
	
}

void MiddleHandleNightLux(int lux){              /*�м�δ���ҹ����նȺ���*/
	
}

void MiddleHandleLux(char time, int lux){        /*�м�δ���ʱ��͹��նȺ���*/
	if(time){
		MiddleHandleDayLux(lux);
	} else {
		MiddleHandleNightLux(lux);
	}	
}

void EntryHandleDayLux(int lux){                 /*��ڶδ��������նȺ���*/
	
}

void EntryHandleNightLux(int lux){               /*��ڶδ���ҹ����նȺ���*/
	
}

void EntryHandleLux(char time, int lux){         /*��ڶδ���ʱ��͹��նȺ���*/
	if(time){
		EntryHandleDayLux(lux);
	} else {
		EntryHandleNightLux(lux);
	}
}

void ExitHandleDayLux(int lux){                  /*���ڶδ��������նȺ���*/
	
}

void ExitHandleNightLux(int lux){                /*���ڶδ���ҹ����նȺ���*/
	
}

void ExitHandleLux(char time, int lux){          /*���ڶδ���ʱ��͹��նȺ���*/
	if(time){
		ExitHandleDayLux(lux);
	} else {
		ExitHandleNightLux(lux);
	}
}

void TransitIHandleDayLux(int lux){              /*��ڹ��ɶδ��������նȺ���*/
	
}

void TransitIHandleNightLux(int lux){            /*��ڹ��ɶδ���ҹ����նȺ���*/
	
}

void TransitIHandleLux(char time, int lux){      /*��ڹ��ɶδ���ʱ��͹��նȺ���*/
	if(time){
		TransitIHandleDayLux(lux);
	} else {
		TransitIHandleNightLux(lux);
	}
}

void TransitIIHandleDayLux(int lux){             /*���ڹ��ɶδ��������նȺ���*/
	
}

void TransitIIHandleNightLux(int lux){           /*���ڹ��ɶδ���ҹ����նȺ���*/
	
}

void TransitIIHandleLux(char time, int lux){     /*���ڹ��ɶδ���ʱ��͹��նȺ���*/
	if(time){
		TransitIIHandleDayLux(lux);
	} else {
		TransitIIHandleNightLux(lux);
	}
	
}

void __handleLux(char tim, int lux){            /*������ݹ��ն����õ������*/
	GuideHandleLux(tim, lux);
	EntryHandleLux(tim, lux);
	TransitIHandleLux(tim, lux);
	MiddleHandleLux(tim, lux);
	TransitIIHandleLux(tim, lux);
	ExitHandleLux(tim, lux);
}

typedef struct{
	char count;               /*�յ��Ĺ��ն�ֵ����*/
	char TimeArea;            /*��ǰ���ڵ�ʱ��*/
	char LuxArea;             /*��ǰ���ڵĹ�ǿ��*/
	int LastLux;              /*�ϴ��յ�12�ι��ն�ֵ��ƽ��ֵ*/
	int ArrayOfLuxValue[12];  /*�������12�ι��նȵ�����*/	
}StoreParam;                                     

static StoreParam __Luxparam = {0, 0, 0, 0};

static int LastLux = 0;                           /*��һ�δ������Ĺ���ǿ��ֵƽ����*/

#define HardLight      10000    /*ǿ��ֵ*/
#define MiddleLight    500      /*�й�ֵ*/
#define WeakLight      50       /*����ֵ*/
#define NoneLight      5       /*�޹�ֵ*/

#define GoodSight      1        /*����ʮ�ֺ�*/
#define CommonSight    2        /*��������*/
#define HardSight      3        /*�ܼ��Ⱥܵ�*/
#define LoseSight      4        /*������*/
#define Blind          5        /*Ϲ����*/

static void DivisiveLightArea(int lux){     /*���ݵ�ǰ���նȣ���������������*/
	char state;
	int val;
	
	if(lux >= HardLight){
		state = GoodSight;
	} else if ((lux >= MiddleLight) && (lux < HardLight)){
		state = CommonSight;
	} else if((lux >= WeakLight) && (lux < MiddleLight)){
		state = HardSight;
	} else if((lux >= NoneLight) && (lux < WeakLight)){
		state = LoseSight;
	} else{
		state = Blind;
	}
	
	if(state != __Luxparam.LuxArea)
		val = abs(lux - __Luxparam.LastLux);
}

extern unsigned char *DayToSunshine(void);

extern unsigned char *DayToNight(void);

#define HaveSun        1     /*����ʱ��*/
#define HaveMoon       2     /*ҹ��ʱ��*/
#define OnOffLight     3     /*���ص�ʱ��*/
#define LateNight      4     /*��ҹʱ��*/

#define  WorkTime  (6 * 60 * 60)         /*���Ϲ�������ʼ����ʱ��*/
#define  RestTime  (10 * 60 * 60)        /*���ǿ�ʼ��Ϣʱ��*/

static void DivideTimeArea(DateTime time){   /*���ݵ�ǰʱ�䣬����������ʱ��*/ 
	int NowTime, DayTime, DarkTime;
	unsigned char *buf;
	
	NowTime = time.hour * 60 * 60 + time.minute * 60 + time.second;    /*��ǰʱ��*/
	
	buf = DayToSunshine();
	DayTime = buf[0]* 60 * 60 + buf[1] * 60 + buf[2];                  /*����ʱ��*/
	
	buf = DayToNight();
	DarkTime = buf[0] * 60 * 60 + buf[1] * 60 + buf[2];                /*���ʱ��*/                             
	
	if((NowTime >= (DayTime + 3600)) && (NowTime <= (DarkTime - 3600))){  /*������һСʱ�����ǰһСʱ*/
		__Luxparam.TimeArea = HaveSun;	                                  /*����Ϊ����ʱ��*/
	} else if((NowTime >= RestTime) && (NowTime <= DayTime) && (NowTime < WorkTime)){ /*��Ϣʱ�����������ǰ��������ʱ��ǰ*/
		__Luxparam.TimeArea = LateNight;                                  /*����Ϊ�������˵���ҹʱ��*/
	} else if(((NowTime > DayTime) && (NowTime < (DayTime + 3600))) || 
						((NowTime < DarkTime) && (NowTime > (DarkTime - 3600)))){  /*���ǰһСʱ������������һСʱ*/
		__Luxparam.TimeArea	= OnOffLight;				                           /*����Ϊ���ص�ʱ�䷶Χ��*/
	} else {
		__Luxparam.TimeArea	= HaveMoon;                                    /*����ʱ�䶨��Ϊ���ǻ�δ��Ϣ��ҹ��ʱ���*/
	}
}

static void HandleLuxGather(ProtocolHead *head, const char *p) {
	char msg[10]; 
	unsigned char *buf, size;
	int LuxValue, second;
	DateTime dateTime;
	char StateChange = 0;           /*���ն��Ƿ�ı���������1λ�ı䣬0Ϊδ��*/
		
  sscanf(p, "%8s", msg);
	LuxValue = atoi((const char *)msg);
	
	second = RtcGetTime();
  SecondToDateTime(&dateTime, second);
	
	DivideTimeArea(dateTime);
	
	__Luxparam.ArrayOfLuxValue[__Luxparam.count] = LuxValue;
	
	__Luxparam.count++;
	if(__Luxparam.count == 12){
		
	}
	if(LastLux)
	
	if(LuxValue > 200000){
		return;
	}

	__handleLux(__Luxparam.TimeArea, LuxValue);
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);			
}

static void HandleRestart(ProtocolHead *head, const char *p){            /*�豸��λ*/
	unsigned char *buf, msg[11], size;
	
	if((p[0] == '1')){
		NVIC_SystemReset();
	} else if(p[0] == '3'){
		sscanf(p, "%10s", msg);
		buf = ProtocolRespond("9999999999", head->contr, (const char *)msg, &size);
		ElecTaskSendData((const char *)buf, size);
		ProtocolDestroyMessage((const char *)buf);	
	}
}

typedef void (*ProtocolHandleFunction)(ProtocolHead *head, const char *p);
typedef struct {
	unsigned char type;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


/*GW: gateway  ����*/
/*EG: electric quantity gather �����ɼ���*/
/* illuminance �� ���ն�*/
/*BSN: �Ƶ�������*/
void ProtocolHandler(ProtocolHead *head, char *p) {
	int i;
	char tmp[12], mold, buf[20];
	char *ret;
	char verify = 0;
	unsigned char len;
	GMSParameter g;
	
	const static ProtocolHandleMap map[] = {  
		{GATEPARAM,      HandleGatewayParam},     /*0x01; ���ز�������*/           ///
		{LIGHTPARAM,     HandleLightParam},       /*0x02; �Ʋ�������*/             /// 
		{DIMMING,        HandleLightDimmer},      /*0x04; �Ƶ������*/
		{LAMPSWITCH,     HandleLightOnOff},       /*0x05; �ƿ��ؿ���*/
		{READDATA,       HandleReadBSNData},      /*0x06; ������������*/
		{DATAQUERY,      HandleGWDataQuery},      /*0x08; �������ݲ�ѯ*/           		    
		{VERSIONQUERY,   HandleGWVersQuery},      /*0x0C; ����������汾��*/       ///
		{GATEUPGRADE,    HandleGWUpgrade},        /*0x37; ����Զ������*/
		{TIMEADJUST,     HandleAdjustTime},       /*0x42; Уʱ*/                   ///  
		{LUXVALUE,       HandleLuxGather},        /*0x43; ���յ����ն�ǿ��ֵ*/		
		{RESTART,        HandleRestart},          /*0x3F; �豸��λ*/               ///  
	};

	ret = p;
	
	for (i = 0; i < (strlen(p) - 3); ++i) {
		verify ^= *ret++;
	}
	
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter)  + 1)/ 2);
	sscanf(p, "%*1s%10s", tmp);
	if((strncmp((const char *)g.GWAddr, tmp, 10) != 0) && (strncmp("9999999999", tmp, 10) != 0)) {
		
		NorFlashWrite(NORFLASH_MANAGEM_ADDR, (const short *)&g, (sizeof(GMSParameter)  + 1)/ 2);
	}
	
	sscanf(p, "%*13s%2s", tmp);
	len = strtol((const char *)tmp, NULL, 16);  
	
	sprintf(buf, "%%*%ds%%2s", 15 + len);
	
	sscanf(p, buf, tmp);
	len = strtol((const char *)tmp, NULL, 16);  
	
	if(verify != len)
		return;
	
	sscanf(p, "%*11s%2s", tmp);
	if(tmp[1] > '9'){
		mold = ((tmp[0] & 0x0f) << 4) | (tmp[1] - 'A' + 10);
	} else {
		mold = ((tmp[0] & 0x0f) << 4) | (tmp[1] & 0x0f);
	}

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
			if (map[i].type == mold) {
				map[i].func(head, p + 15);
				break;
			}
	}
}

