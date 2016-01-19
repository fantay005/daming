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
	ACKERROR = 0,           /*从站应答异常*/
	GATEPARAM = 0x01,       /*网关参数下载*/
	LIGHTPARAM = 0x02,      /*灯参数下载*/
	DIMMING = 0x04,         /*灯调光控制*/
	LAMPSWITCH = 0x05,      /*灯开关控制*/
	READDATA = 0x06,        /*读镇流器数据*/
	LOOPCONTROL = 0x07,     /*网关回路控制*/
	DATAQUERY = 0x08,       /*网关数据查询*/
	VERSIONQUERY = 0x0C,    /*网关软件版本号查询*/ 
  ELECTRICGATHER = 0x0E,  /*电量采集软件版本号查询*/	
	GATEUPGRADE = 0x37,     /*网关远程升级*/
	TIMEADJUST = 0x42,      /*校时*/
	LUXVALUE = 0x43,        /*接收到光强度值*/
	RESTART = 0x3F,         /*设备复位*/
	RETAIN,                 /*保留*/
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

	if(p[0] == '1'){          /*增加一盏灯*/
		
		len = (strlen(p) - 1) / 17;

		for(i = 0; i < len; i++) {
			__ParamWriteToFlash(&p[1 + i * 17]);
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4])); 
		}
		msg[i * 4] = p[0];
		
	} else if (p[0] == '2'){   /*删除一盏灯*/
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
	ENTRANCE = 1,       /*入口段*/
	TRANSITION_I,       /*入口过渡段*/
	INTERMEDIATE,       /*中间段*/
	TRANSITON_II,       /*出口过渡段*/
	EXITSECTION,        /*出口段*/
	GUIDESECTION,       /*引导段*/
}PartType;            /*隧道各段定义*/

#define NoChoice     0           /*属性没有选择*/

void SeekAddress(PartType loop, unsigned short pole, char main, char aux){     /*根据灯参查找zigbee地址*/
	int i = 0, j, len = 0, m, n;
	unsigned char tmp[5], *ret, *msg;
	Lightparam k;
	
	msg = DataFalgQueryAndChange(2, 0, 1);

	memset(__msg.ArrayAddr, 0, 600);
	for(len = 0; len < 599; len++) {
		NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
		
		if(k.Loop != (loop + '0')){            /*回路要一致*/
			continue;
		}
		
		sscanf((const char *)k.LightPole, "%4s", tmp);
		m = atoi((const char *)tmp);           /*灯杆号*/
		
		if((pole != NoChoice) && (m != pole)){
			continue;
		}
		
		sscanf((const char *)k.Attribute, "%2s", tmp);
		m = atoi((const char *)tmp);           /*主辅道属性*/
		
		if(m < 0x10){                          /*主道灯*/
			
			if((main != NoChoice) && (main != (m & 0x0F)))
				continue;
		} else if((m > 0x10) && (m < 0x20)){   /*辅道灯*/
			
			if((aux != NoChoice) && (aux != (m & 0x0F)))
				continue;
		} else if(m > 0x30){                   /*投光灯*/
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
				
				if(k.Attribute[0] == '0'){                               /*主道灯*/
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
				} else if(k.Attribute[0] == '1'){	                      /*辅道灯*/
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
				} else {                                               /*投光灯*/
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
				
				if(k.Attribute[0] == '0'){                           /*主道灯*/
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
				} else if(k.Attribute[0] == '1'){	                   /*辅道灯*/
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

static void HandleGWDataQuery(ProtocolHead *head, const char *p) {     /*网关回路数据查询*/
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

static void HandleAdjustTime(ProtocolHead *head, const char *p) {    /*校时*/
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

static void HandleGWVersQuery(ProtocolHead *head, const char *p) {      /*查网关软件版本号*/
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

static void HandleGWUpgrade(ProtocolHead *head, const char *p) {             //FTP远程升级
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

void GuideHandleDayLux(int lux){                 /*引导段处理白天光照度函数*/

}

void GuideHandleNightLux(int lux){               /*引导段处理夜晚光照度函数*/
	
}

void GuideHandleLux(char time, int lux){         /*引导段处理时间和光照度函数*/
	if(time){
		GuideHandleDayLux(lux);
	} else {
		GuideHandleNightLux(lux);
	}	
}

void MiddleHandleDayLux(int lux){                /*中间段处理白天光照度函数*/
	
}

void MiddleHandleNightLux(int lux){              /*中间段处理夜晚光照度函数*/
	
}

void MiddleHandleLux(char time, int lux){        /*中间段处理时间和光照度函数*/
	if(time){
		MiddleHandleDayLux(lux);
	} else {
		MiddleHandleNightLux(lux);
	}	
}

void EntryHandleDayLux(int lux){                 /*入口段处理白天光照度函数*/
	
}

void EntryHandleNightLux(int lux){               /*入口段处理夜晚光照度函数*/
	
}

void EntryHandleLux(char time, int lux){         /*入口段处理时间和光照度函数*/
	if(time){
		EntryHandleDayLux(lux);
	} else {
		EntryHandleNightLux(lux);
	}
}

void ExitHandleDayLux(int lux){                  /*出口段处理白天光照度函数*/
	
}

void ExitHandleNightLux(int lux){                /*出口段处理夜晚光照度函数*/
	
}

void ExitHandleLux(char time, int lux){          /*出口段处理时间和光照度函数*/
	if(time){
		ExitHandleDayLux(lux);
	} else {
		ExitHandleNightLux(lux);
	}
}

void TransitIHandleDayLux(int lux){              /*入口过渡段处理白天光照度函数*/
	
}

void TransitIHandleNightLux(int lux){            /*入口过渡段处理夜晚光照度函数*/
	
}

void TransitIHandleLux(char time, int lux){      /*入口过渡段处理时间和光照度函数*/
	if(time){
		TransitIHandleDayLux(lux);
	} else {
		TransitIHandleNightLux(lux);
	}
}

void TransitIIHandleDayLux(int lux){             /*出口过渡段处理白天光照度函数*/
	
}

void TransitIIHandleNightLux(int lux){           /*出口过渡段处理夜晚光照度函数*/
	
}

void TransitIIHandleLux(char time, int lux){     /*出口过渡段处理时间和光照度函数*/
	if(time){
		TransitIIHandleDayLux(lux);
	} else {
		TransitIIHandleNightLux(lux);
	}
	
}

void __handleLux(char tim, int lux){            /*白天根据光照度设置调光策略*/
	GuideHandleLux(tim, lux);
	EntryHandleLux(tim, lux);
	TransitIHandleLux(tim, lux);
	MiddleHandleLux(tim, lux);
	TransitIIHandleLux(tim, lux);
	ExitHandleLux(tim, lux);
}

typedef struct{
	char count;               /*收到的光照度值计数*/
	char TimeArea;            /*当前所在的时域*/
	char LuxArea;             /*当前所在的光强域*/
	int LastLux;              /*上次收到12次光照度值的平均值*/
	int ArrayOfLuxValue[12];  /*记载最近12次光照度的数组*/	
}StoreParam;                                     

static StoreParam __Luxparam = {0, 0, 0, 0};

static int LastLux = 0;                           /*上一次传进来的光照强度值平均数*/

#define HardLight      10000    /*强光值*/
#define MiddleLight    500      /*中光值*/
#define WeakLight      50       /*弱光值*/
#define NoneLight      5       /*无光值*/

#define GoodSight      1        /*视线十分好*/
#define CommonSight    2        /*正常视线*/
#define HardSight      3        /*能见度很低*/
#define LoseSight      4        /*看不清*/
#define Blind          5        /*瞎眼了*/

static void DivisiveLightArea(int lux){     /*根据当前光照度，区分其所在区域*/
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

#define HaveSun        1     /*白天时段*/
#define HaveMoon       2     /*夜晚时段*/
#define OnOffLight     3     /*开关灯时段*/
#define LateNight      4     /*深夜时段*/

#define  WorkTime  (6 * 60 * 60)         /*早上公交车开始工作时间*/
#define  RestTime  (10 * 60 * 60)        /*人们开始休息时间*/

static void DivideTimeArea(DateTime time){   /*根据当前时间，划分其所属时域*/ 
	int NowTime, DayTime, DarkTime;
	unsigned char *buf;
	
	NowTime = time.hour * 60 * 60 + time.minute * 60 + time.second;    /*当前时间*/
	
	buf = DayToSunshine();
	DayTime = buf[0]* 60 * 60 + buf[1] * 60 + buf[2];                  /*破晓时间*/
	
	buf = DayToNight();
	DarkTime = buf[0] * 60 * 60 + buf[1] * 60 + buf[2];                /*天黑时间*/                             
	
	if((NowTime >= (DayTime + 3600)) && (NowTime <= (DarkTime - 3600))){  /*天亮后一小时到天黑前一小时*/
		__Luxparam.TimeArea = HaveSun;	                                  /*定义为白天时段*/
	} else if((NowTime >= RestTime) && (NowTime <= DayTime) && (NowTime < WorkTime)){ /*休息时间后到早上六点前，且天亮时间前*/
		__Luxparam.TimeArea = LateNight;                                  /*定义为少有行人的深夜时段*/
	} else if(((NowTime > DayTime) && (NowTime < (DayTime + 3600))) || 
						((NowTime < DarkTime) && (NowTime > (DarkTime - 3600)))){  /*天黑前一小时，或者天亮后一小时*/
		__Luxparam.TimeArea	= OnOffLight;				                           /*定义为开关灯时间范围内*/
	} else {
		__Luxparam.TimeArea	= HaveMoon;                                    /*其他时间定义为人们还未休息的夜晚时间段*/
	}
}

static void HandleLuxGather(ProtocolHead *head, const char *p) {
	char msg[10]; 
	unsigned char *buf, size;
	int LuxValue, second;
	DateTime dateTime;
	char StateChange = 0;           /*光照度是否改变所在区域，1位改变，0为未改*/
		
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

static void HandleRestart(ProtocolHead *head, const char *p){            /*设备复位*/
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


/*GW: gateway  网关*/
/*EG: electric quantity gather 电量采集器*/
/* illuminance ： 光照度*/
/*BSN: 钠灯镇流器*/
void ProtocolHandler(ProtocolHead *head, char *p) {
	int i;
	char tmp[12], mold, buf[20];
	char *ret;
	char verify = 0;
	unsigned char len;
	GMSParameter g;
	
	const static ProtocolHandleMap map[] = {  
		{GATEPARAM,      HandleGatewayParam},     /*0x01; 网关参数下载*/           ///
		{LIGHTPARAM,     HandleLightParam},       /*0x02; 灯参数下载*/             /// 
		{DIMMING,        HandleLightDimmer},      /*0x04; 灯调光控制*/
		{LAMPSWITCH,     HandleLightOnOff},       /*0x05; 灯开关控制*/
		{READDATA,       HandleReadBSNData},      /*0x06; 读镇流器数据*/
		{DATAQUERY,      HandleGWDataQuery},      /*0x08; 网关数据查询*/           		    
		{VERSIONQUERY,   HandleGWVersQuery},      /*0x0C; 查网关软件版本号*/       ///
		{GATEUPGRADE,    HandleGWUpgrade},        /*0x37; 网关远程升级*/
		{TIMEADJUST,     HandleAdjustTime},       /*0x42; 校时*/                   ///  
		{LUXVALUE,       HandleLuxGather},        /*0x43; 接收到光照度强度值*/		
		{RESTART,        HandleRestart},          /*0x3F; 设备复位*/               ///  
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

