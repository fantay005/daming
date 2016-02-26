#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
//#include "sms.h"
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
	STRATEGY = 0x03,        /*��������*/
	DIMMING = 0x04,         /*�Ƶ������*/
	LAMPSWITCH = 0x05,      /*�ƿ��ؿ���*/
	READDATA = 0x06,        /*������������*/
	LOOPCONTROL = 0x07,     /*���ػ�·����*/
	DATAQUERY = 0x08,       /*�������ݲ�ѯ*/
	TIMEQUERY = 0x09,       /*���ؿ��ص�ʱ���ѯ*/
	AUTOWORK = 0x0A,        /*����������*/
	TIMEADJUST = 0x0B,      /*Уʱ*/
	VERSIONQUERY = 0x0C,    /*��������汾�Ų�ѯ*/ 
  ELECTRICGATHER = 0x0E,  /*�����ɼ�����汾�Ų�ѯ*/	
	ADDRESSQUERY = 0x11,    /*���ص�ַ��ѯ*/
	SETSERVERIP = 0x14,     /*��������Ŀ�������IP*/
	GATEUPGRADE = 0x15,     /*����Զ������*/
	RSSIVALUE = 0x17,       /*GSM�ź�ǿ�Ȳ�ѯ*/
	CALLBALANCE = 0x18,     /*????????*/
	GATHERUPGRADE = 0x1E,   /*�����ɼ�ģ��Զ������*/
	BALLASTUPGRADE= 0x2A,   /*������Զ������*/
	RESTART = 0x3F,         /*�豸��λ*/
	RETAIN,                 /*����*/
} GatewayType;

typedef struct {
	unsigned char BCC[2];
	unsigned char x03;
} ProtocolTail;

void CurcuitContrInit(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CRTL_EN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_EN, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_1, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_1, PIN_CTRL_1);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_2, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_2, PIN_CTRL_2);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_3, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_3, PIN_CTRL_3);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_4, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_4, PIN_CTRL_4);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_5, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_5, PIN_CTRL_5);

	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_6, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_6, PIN_CTRL_6);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_7, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_7, PIN_CTRL_7);
	
	GPIO_InitStructure.GPIO_Pin =  PIN_CTRL_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CTRL_8, &GPIO_InitStructure);
	GPIO_ResetBits(GPIO_CTRL_8, PIN_CTRL_8);	

	GPIO_ResetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
}

unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret, tmp[5];
	unsigned char hexTable[] = "0123456789ABCDEF";
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 9 + len + 3 + 2;
	
	ret = pvPortMalloc(*size + 1);
	
	
	if(strncmp((const char *)address, "FFFF", 4) == 0){
		*ret = 0xFF;
		*(ret + 1) = 0xFF;
	} else {
		
#if defined (__HEXADDRESS__)	
		sscanf((const char *)address, "%4s", tmp);
		verify = strtol((const char *)tmp, NULL, 16);
		*ret = verify >> 8;
		*(ret + 1) = verify & 0xFF;
#else		
		*ret = (address[0] << 4) | (address[1] & 0x0f);
		*(ret + 1) = (address[2] << 4) | (address[3] & 0x0f);
#endif		
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
		sscanf(p, "%*30s%2s", g.NumbOfLoop);
		g.EmbedInformation = 1;
		NorFlashWrite(NORFLASH_MANAGEM_BASE, (const short *)&g, (sizeof(GatewayParam1) + 1) / 2);
	} else if(strlen(p) == (27 - 15)){
		GatewayParam2 g;
		sscanf(p, "%*1s%2s", g.OpenOffsetTime1);
		sscanf(p, "%*3s%2s", g.OpenOffsetTime2);
		sscanf(p, "%*5s%2s", g.CloseOffsetTime1);
		sscanf(p, "%*7s%2s", g.CloseOffsetTime2);
		g.SetFlag = 1;
	//	NorFlashWriteChar(NORFLASH_MANAGEM_TIMEOFFSET, (const char *)g, sizeof(GatewayParam2));
		NorFlashWrite(NORFLASH_MANAGEM_TIMEOFFSET, (const short *)&g, (sizeof(GatewayParam2) + 1) / 2);
	} else if(strlen(p) == (78 - 15)){
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

void StoreZigbAddr(unsigned char rank, short Address){
	unsigned short End[96], i;
	NorFlashRead(NORFLASH_END_LIGHT_ADDR , (short *)End, sizeof(End)/sizeof(short));
	for(i = 0; i < 6; i++){
		if(End[rank * 6 + i] == 0xFFFF){
			End[rank * 6 + i] = Address;
			break;
		}
	}
	NorFlashWrite(NORFLASH_END_LIGHT_ADDR , (short *)End, sizeof(End)/sizeof(short));
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
		sscanf(p, "%*16s%2s", g.Attribute);
		
		sprintf((char *)g.TimeOfSYNC, "%02d%02d%02d%02d%02d%02d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
  
		g.CommState = 0x04;
		g.InputPower = 0;
		
	//	NorFlashWriteChar(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const char *)g, sizeof(Lightparam));
		
		sscanf(p, "%4s",msg);

#if defined(__HEXADDRESS__)
		len = strtol((const char *)msg, NULL, 16);
#else				
		len = atoi((const char *)msg);
#endif		
		
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(Lightparam) + 1) / 2);	
}

static void HandleLightParam(ProtocolHead *head, const char *p) {
	int len, i, lenth;
	unsigned char size;
	DateTime dateTime;
	uint32_t second;	
	unsigned char *buf, msg[42], tmp[5];
	Lightparam g;
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);

	if(p[0] == '1'){          /*����һյ��*/
		
		len = (strlen(p) - 18) / 17;
		memset(msg, 0, 41);
		for(i = 0; i < len; i++) {
			__ParamWriteToFlash(&p[1 + i * 17]);
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4]));
		}
		msg[i * 4] = p[0];

		
	} else if (p[0] == '2'){   /*ɾ��һյ��*/
		len = (strlen(p) - 18) / 17;
		memset(msg, 0, 41);
		for(i = 0; i < len; i++) {
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4]));
			sscanf(&p[1 + i * 17], "%4s", tmp);
		
#if defined(__HEXADDRESS__)
			lenth = strtol((const char *)tmp, NULL, 16);
#else				
			lenth = atoi((const char *)tmp);
#endif		
		
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + lenth * NORFLASH_SECTOR_SIZE);
		}
		msg[i * 4 + 1] = p[0];
		
	} else  if (p[0] == '3'){  /*����һյ��*/
		sscanf(p, "%*5s%4s", g.AddrOfZigbee);
		sscanf(p, "%*5s%4s",msg);
	//	msg[4] = 0;
		len = atoi((const char *)msg);
		NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		sscanf(p, "%*9s%4s", g.AddrOfZigbee);
		sscanf(p, "%*5s%4s",msg);
	//	msg[4] = 0;
		
#if defined(__HEXADDRESS__)
		len = strtol((const char *)msg, NULL, 16);
#else				
		len = atoi((const char *)msg);
#endif		
		
		sscanf(p, "%*13s%4s", g.NorminalPower);
//		sscanf(p, "%*16s%12s", g->Loop);
		g.Loop = p[17];
		sscanf(p, "%*18s%4s", g.LightPole);
//		sscanf(p, "%*16s%12s", g->LightSourceType);
		g.LightSourceType = p[22];
//		sscanf(p, "%*16s%12s", g->LoadPhaseLine);
		g.LoadPhaseLine = p[23];
		sscanf(p, "%*24s%2s", g.Attribute);
		
    sprintf((char *)g.TimeOfSYNC, "%2d%2d%2d%2d%2d%2d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
	  for(i = 0; i < 12; i++){
			if(g.TimeOfSYNC[i] == 0x20){
				g.TimeOfSYNC[i] = '0';
			}
		}
		
		g.CommState = 0x04;
	  g.InputPower = 0;
		
//		buf = (unsigned char *)&g;
//		memset(tmp, 0, 255);
//		for(i=0; i<sizeof(Lightparam); i++){
//			tmp[i] = buf[i];
//		}
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(Lightparam) + 1) / 2);
		

	} else if (p[0] == '4'){
		for(len = 0; len < 1000; len++){
			NorFlashEraseParam(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE);
		}
	}
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
//	if(Increase == 1){
//		NorFlashRead(NORFLASH_LIGHT_NUMBER , (short *)&Total, 1);
//		if(Total[0] != 0xFFFF){
//			Total[0] += 1;
//		} else {
//			Total[0] = 1;
//		}
//		NorFlashWrite(NORFLASH_LIGHT_NUMBER , (const short *)&Total, 1);
//		
//		sscanf((const char *)g.LightPole, "%4s", msg);
//		len = atoi((const char *)msg) - 9001;
//		
//		sscanf((const char *)g.AddrOfZigbee, "%4s", msg);
//		i = atoi((const char *)msg);
//		StoreZigbAddr(len, i);
//	}
}

static void HandleStrategy(ProtocolHead *head, const char *p) {
	int len, i;
	unsigned char *buf, msg[10], size, *ret;
	StrategyParam g;
	Lightparam k;
	DateTime dateTime;
	uint32_t second;	
	
	ret = (unsigned char *)(p + 4);
//	sscanf((const char *)ret, "%4s", g.AddrOfZigb);
	sscanf((const char *)ret, "%*4s%2s", g.SchemeType);
//	sscanf(p, "%*15s%4s", g->DimmingNOS);

//	for(len= 0; len < 12; len++){
//		if(len%2 == 0){
//			time[len] = (buf[len/2] >> 4) + '0';
//		} else {
//			time[len] = (buf[len/2] & 0x0F) + '0';
//		}
//	}
//	strcpy((char *)g.AddrOfZigb, (const char *)time);
	g.DimmingNOS = ret[6];

	sscanf((const char *)ret, "%*7s%4s", g.FirstDCTime);
	sscanf((const char *)ret, "%*11s%2s", g.FirstDPVal);
	
	if((g.DimmingNOS - '0') > 1) {
		sscanf((const char *)ret, "%*13s%4s", g.SecondDCTime);
		sscanf((const char *)ret, "%*17s%2s", g.SecondDPVal);
	} else {
		sscanf("FFFF", "%4s", g.SecondDCTime);
		sscanf("FFFF", "%2s", g.SecondDPVal);
	}
	
	if((g.DimmingNOS - '0') > 2) {
		sscanf((const char *)ret, "%*19s%4s", g.ThirdDCTime);
		sscanf((const char *)ret, "%*23s%2s", g.ThirdDPVal);
	} else {
		sscanf("FFFF", "%4s", g.ThirdDCTime);
		sscanf("FFFF", "%2s", g.ThirdDPVal);		
	}
	
	if((g.DimmingNOS - '0') > 3){
		sscanf((const char *)ret, "%*25s%4s", g.FourthDCTime);
		sscanf((const char *)ret, "%*29s%2s", g.FourthDPVal);
	} else {
		sscanf("FFFF", "%4s", g.FourthDCTime);
		sscanf("FFFF", "%2s", g.FourthDPVal);				
	}
	
	if((g.DimmingNOS - '0')> 4){
		sscanf((const char *)ret, "%*31s%4s", g.FifthDCTime);
		sscanf((const char *)ret, "%*35s%2s", g.FifthDPVal);
	} else {
		sscanf("FFFF", "%4s", g.FifthDCTime);
		sscanf("FFFF", "%2s", g.FifthDPVal);				
	}
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);
	
	sprintf((char *)g.SYNCTINE, "%2d%2d%2d%2d%2d%2d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
	
	for(i = 0; i < 12; i++){
		if(g.SYNCTINE[i] == 0x20){
			g.SYNCTINE[i] = '0';
		}
	}
	//NorFlashWriteShort(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)g, sizeof(StrategyParam));
	
	if(p[0] == 'A'){
		
		for(i = 0; i < 1000; i++){
			NorFlashRead(NORFLASH_BALLAST_BASE + i * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if((k.Loop > '0') && (k.Loop < '9')){
				sscanf((const char *)k.AddrOfZigbee, "%4s", msg);
				sscanf((const char *)k.AddrOfZigbee, "%4s", g.AddrOfZigb);
				g.SchemeType[0] = ret[4];
				
#if defined(__HEXADDRESS__)
				len = strtol((const char *)msg, NULL, 16);
#else				
				len = atoi((const char *)msg);
#endif					
				
				NorFlashWrite(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(StrategyParam) + 1) / 2);
			}
		}
	} else if(p[0] == 'B'){
		
		sscanf((const char *)ret, "%4s", msg);
		sscanf((const char *)ret, "%4s", g.AddrOfZigb);
		g.SchemeType[0] = ret[4];
		
#if defined(__HEXADDRESS__)
		len = strtol((const char *)msg, NULL, 16);
#else				
		len = atoi((const char *)msg);
#endif			
		NorFlashWrite(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(StrategyParam) + 1) / 2);
		
	} else if(p[0] == '9'){
		
		for(i = 0; i < 1000; i++){
			NorFlashRead(NORFLASH_BALLAST_BASE + i * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if(k.Attribute[0] == '0'){
				sscanf((const char *)k.AddrOfZigbee, "%4s", msg);
				sscanf((const char *)k.AddrOfZigbee, "%4s", g.AddrOfZigb);
				g.SchemeType[0] = ret[4];
				
#if defined(__HEXADDRESS__)
				len = strtol((const char *)msg, NULL, 16);
#else				
				len = atoi((const char *)msg);
#endif					
				
				NorFlashWrite(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(StrategyParam) + 1) / 2);
			}
		}
		
	} else if(p[0] == '8'){
		for(i = 0; i < 1000; i++){
			NorFlashRead(NORFLASH_BALLAST_BASE + i * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if(k.Attribute[0] == '1'){
				sscanf((const char *)k.AddrOfZigbee, "%4s", msg);
				sscanf((const char *)k.AddrOfZigbee, "%4s", g.AddrOfZigb);
				g.SchemeType[0] = ret[4];
				
#if defined(__HEXADDRESS__)
				len = strtol((const char *)msg, NULL, 16);
#else				
				len = atoi((const char *)msg);
#endif					
				
				NorFlashWrite(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(StrategyParam) + 1) / 2);
			}
		}
		
		
	} else if(p[0] == '7'){
		
		for(i = 0; i < 1000; i++){
			NorFlashRead(NORFLASH_BALLAST_BASE + i * NORFLASH_SECTOR_SIZE, (short *)&k, (sizeof(Lightparam) + 1) / 2);
			if((k.Attribute[0] >= '6') && (k.Attribute[0] <= '9')){
				sscanf((const char *)k.AddrOfZigbee, "%4s", msg);
				sscanf((const char *)k.AddrOfZigbee, "%4s", g.AddrOfZigb);
				g.SchemeType[0] = ret[5];
				
#if defined(__HEXADDRESS__)
				len = strtol((const char *)msg, NULL, 16);
#else				
				len = atoi((const char *)msg);
#endif					
				
				NorFlashWrite(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(StrategyParam) + 1) / 2);
			}
		}
		
	}

	
	
	
	NorFlashWrite(NORFLASH_STRATEGY_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(StrategyParam) + 1) / 2);
	
	sscanf(p, "%8s", msg);
	
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

static void HandleGWloopControl(ProtocolHead *head, const char *p) {
	unsigned char tmp[5] = {0}, a, b, TurnFlag = 0;
	unsigned char *buf, size;
	int len;
	
//	memset(tmp, 0, 3);
	if(p[0] == '0'){              /*ǿ�ƿ�*/  
		TurnFlag = 1;
	} else if(p[0] == '1'){       /*���Կ�*/
		uint32_t second, OnOffSecond, OffTime1,OffTime2;
		GatewayParam2 g;
		unsigned short msg[1465];
	  DateTime dateTime;
		uint32_t BaseSecond = (7 * (365 + 365 + 365 + 366) + 2 * 365) * 24 * 60 * 60;
		
		second = RtcGetTime();
		SecondToDateTime(&dateTime, second);
		
		len = __OffsetNumbOfDay(&dateTime);
		if(dateTime.year % 2){
			NorFlashRead(NORFLASH_ONOFFTIME1, (short *)msg, len * 4);
		} else {
			NorFlashRead(NORFLASH_ONOFFTIME2, (short *)msg, len * 4);
		}
		
		NorFlashRead(NORFLASH_MANAGEM_TIMEOFFSET, (short *)&g, (sizeof(GatewayParam2) + 1) / 2);
		
		if(dateTime.hour < 12) {
			OnOffSecond = (msg[len * 4 - 4] << 16) + msg[len * 4 - 3];
			
			sscanf((const char *)g.OpenOffsetTime1, "%2s", tmp);
			a = strtol((const char *)buf, NULL, 16);
			
			sscanf((const char *)g.OpenOffsetTime2, "%2s", tmp);
			b = strtol((const char *)buf, NULL, 16);
		
			if(a & 0x80){
				OffTime1 = OnOffSecond + (a & 0x7f) * 60;
			} else {
				OffTime1 = OnOffSecond - (a & 0x7f) * 60;
			}
			
			if(b & 0x80){
				OffTime2 = OnOffSecond + (b & 0x7f) * 60;
			} else {
				OffTime2 = OnOffSecond - (b & 0x7f) * 60;
			}
			
			if((second >= OffTime1) && (second <= OffTime2)){
				TurnFlag = 1;
				
				msg[len * 4 - 4] = (second + BaseSecond) >> 16;
				msg[len * 4 - 3] = (second + BaseSecond) & 0xFFFF;
				
				if(dateTime.year % 2){
					NorFlashWrite(NORFLASH_ONOFFTIME1, (short *)msg, (len * 4 - 2));
				} else {
					NorFlashWrite(NORFLASH_ONOFFTIME2, (short *)msg, (len * 4 - 2));
				}
				
			}
		} else {
			OnOffSecond = (msg[len * 4 - 2] << 16) + msg[len * 4 - 1];
			
			sscanf((const char *)g.CloseOffsetTime1, "%2s", tmp);
			a = strtol((const char *)buf, NULL, 16);
			
			sscanf((const char *)g.CloseOffsetTime2, "%2s", tmp);
			b = strtol((const char *)buf, NULL, 16);
			
			if(a & 0x80){
				OffTime1 = OnOffSecond + (a & 0x7f) * 60;
			} else {
				OffTime1 = OnOffSecond - (a & 0x7f) * 60;
			}
			
			if(b & 0x80){
				OffTime2 = OnOffSecond + (b & 0x7f) * 60;
			} else {
				OffTime2 = OnOffSecond - (b & 0x7f) * 60;
			}
			
			if((second >= OffTime1) && (second <= OffTime2)){
				TurnFlag = 1;
				
				msg[len * 4 - 2] = (second + BaseSecond) >> 16;
				msg[len * 4 - 1] = (second + BaseSecond) & 0xFFFF;
				
				if(dateTime.year % 2){
					NorFlashWrite(NORFLASH_ONOFFTIME1, (short *)msg, len * 4);
				} else {
					NorFlashWrite(NORFLASH_ONOFFTIME2, (short *)msg, len * 4);
				}
			}
		}	
	}
	
	sscanf(p, "%*1s%2s", tmp);
	if((tmp[0] >= '0') && (tmp[0] <= '9')){
		b = (tmp[0] - '0') << 4;
	} else if ((tmp[0] >= 'A') && (tmp[0] <= 'F')){
		b = (tmp[0] - '7') << 4;
	} else {
		return;
	}
	
	if((tmp[1] >= '0') && (tmp[1] <= '9')){
		b |= (tmp[1] - '0');
	} else if ((tmp[1] >= 'A') && (tmp[1] <= 'F')){
		b |= (tmp[1] - '7');
	} else {
		return;
	}

	GPIO_SetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
	
	if(TurnFlag == 1){
		if(b & 0x01){		
				GPIO_SetBits(GPIO_CTRL_1, PIN_CTRL_1);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_1, PIN_CTRL_1);
		}
		if(b & 0x02) {
				GPIO_SetBits(GPIO_CTRL_2, PIN_CTRL_2);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_2, PIN_CTRL_2);
		}
		if(b & 0x04) {
				GPIO_SetBits(GPIO_CTRL_3, PIN_CTRL_3);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_3, PIN_CTRL_3);
		}
		if(b & 0x08) {
				GPIO_SetBits(GPIO_CTRL_4, PIN_CTRL_4);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_4, PIN_CTRL_4);
		}
		if(b & 0x10) {
				GPIO_SetBits(GPIO_CTRL_5, PIN_CTRL_5);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_5, PIN_CTRL_5);
		}
		if(b & 0x20) {
				GPIO_SetBits(GPIO_CTRL_6, PIN_CTRL_6);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_6, PIN_CTRL_6);
		}
		if(b & 0x40) {
				GPIO_SetBits(GPIO_CTRL_7, PIN_CTRL_7);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_7, PIN_CTRL_7);
		}
		if(b & 0x80) {
				GPIO_SetBits(GPIO_CTRL_8, PIN_CTRL_8);
		} else {
			  GPIO_ResetBits(GPIO_CTRL_8, PIN_CTRL_8);
		}
	}
	
	GPIO_ResetBits(GPIO_CTRL_EN, PIN_CRTL_EN);
	TurnFlag = 0;
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)tmp, &size);
  GsmTaskSendTcpData((const char *)buf, size);
 	ProtocolDestroyMessage((const char *)buf);	
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

static void HandleGWTurnTimeQuery(ProtocolHead *head, const char *p) {
	int len;
	unsigned char *buf, msg[18], size;
	unsigned short tmp[1465];
	DateTime dateTime;
	
	if(p[0] != '1'){
		buf = ProtocolRespond(head->addr, head->contr, "0", &size);
		GsmTaskSendTcpData((const char *)buf, size);
		ProtocolDestroyMessage((const char *)buf);	
		return;
	}
	
	sscanf(p, "%*1s%4s", msg);
	dateTime.year = atoi((const char *)msg) - 2000;
	sscanf(p, "%*5s%2s", msg);
	dateTime.month =  atoi((const char *)msg);
	sscanf(p, "%*7s%2s", msg);
	dateTime.date = atoi((const char *)msg);
	
	len = __OffsetNumbOfDay(&dateTime);
	
	if(dateTime.year % 2){
		NorFlashRead(NORFLASH_ONOFFTIME1, (short *)tmp, len * 4);
	} else {
		NorFlashRead(NORFLASH_ONOFFTIME2, (short *)tmp, len * 4);
	}
	
	memset(msg, 0, 18);
	sprintf((char *)msg, "1%4x%4x%4x%4x", (unsigned short)tmp[len * 4 - 2], (unsigned short)tmp[len * 4 - 1], (unsigned short)tmp[len * 4 - 4], (unsigned short)tmp[len * 4 - 3]);
	
	for(len = 0; len < sizeof(msg); len++){
		if(msg[len] == 0x20){
			msg[len] = 0x30;
		}
	}
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleLightAuto(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static void HandleAdjustTime(ProtocolHead *head, const char *p) {    /*Уʱ*/
	DateTime ServTime;
	uint32_t second;
	
	second = RtcGetTime();
	
	ServTime.year = (p[0] - '0') * 10 + p[1] - '0';
	ServTime.month = (p[2] - '0') * 10 + p[3] - '0';
	ServTime.date = (p[4] - '0') * 10 + p[5] - '0';
	ServTime.hour = (p[6] - '0') * 10 + p[7] - '0';
	ServTime.minute = (p[8] - '0') * 10 + p[9] - '0';
	ServTime.second = (p[10] - '0') * 10 + p[11] - '0';
	
	if(DateTimeToSecond(&ServTime) > second){
		if((DateTimeToSecond(&ServTime) - second) > 300) {
			RtcSetTime(DateTimeToSecond(&ServTime));
		}	
	} else if (DateTimeToSecond(&ServTime) < second){
		if((second - DateTimeToSecond(&ServTime)) > 300){
			RtcSetTime(DateTimeToSecond(&ServTime));
		}
	}
	
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

static char ConectServer = 0;

char Conect_server_start(void){
	if(ConectServer == 1){
		ConectServer = 0;
		return 1;
	}
	return 0;
}

static void HandleGWAddrQuery(ProtocolHead *head, const char *p) {   /*���ص�ַ��ѯ*/
	unsigned char *buf, msg[5], size;	
	
	sscanf(p, "%4s", msg);
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	ConectServer = 1;
}

extern bool __GPRSmodleReset(void);

static void HandleSetGWServ(ProtocolHead *head, const char *p) {      /*��������Ŀ�������IP*/
	unsigned char *buf, size;
	GMSParameter g;
	
	sscanf(p, "%[^,]", g.serverIP);
	sscanf(p, "%*[^,]%*c%d", &(g.serverPORT));

//	g.serverPORT = atoi((const char *)msg);
	
	NorFlashWrite(NORFLASH_MANAGEM_WARNING, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	while(!__GPRSmodleReset());	
}


static void HandleGWUpgrade(ProtocolHead *head, const char *p) {             //FTPԶ������
	const char *remoteFile = "STM32.PAK";
	unsigned short port = 21;
	FirmwareUpdaterMark *mark;
	char *host, tmp[3];
	int len;
	
	sscanf((const char *)head->lenth, "%2s", tmp);
	strtol((const char *)tmp, NULL, 16);
	host = (char *)pvPortMalloc(len - 5);
	memcpy(host, (p+ 6), (len - 6));
	mark = pvPortMalloc(sizeof(*mark));
	if (mark == NULL) {
		return;
	}

	if (FirmwareUpdateSetMark(mark, host, port, remoteFile, 1)) {
		NVIC_SystemReset();
	}
	vPortFree(mark);
}

extern bool GsmTaskSendAtCommand(const char *atcmd);

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

static void HandleRestart(ProtocolHead *head, const char *p){            /*�豸��λ*/
	unsigned char *buf, msg[11], size;
	
	if((p[0] == '1')){
		NVIC_SystemReset();
	} else if(p[0] == '2') {
		while(!__GPRSmodleReset());
	}else if(p[0] == '3'){
		sscanf(p, "%10s", msg);
		buf = ProtocolRespond("9999999999", head->contr, (const char *)msg, &size);
		ElecTaskSendData((const char *)buf, size);
		ProtocolDestroyMessage((const char *)buf);	
	}
}

extern bool GsmTaskSendAtCommand(const char *atcmd);
extern void SwitchCommand(void);

static void HandleRSSIQuery(ProtocolHead *head, const char *p) {           //GSM????
	SwitchCommand();	
	GsmTaskSendAtCommand("AT+CSQ\r");
}

extern void __cmd_QUERYFARE_Handler(void);
extern void __cmd_QUERYFLOW_Handler(void);

static void HandleCallBalance(ProtocolHead *head, const char *p) {        /*???????*/
	char tmp[8];
	
	sscanf(p, "%2s", tmp);
	if(strncmp(tmp, "00", 2) == 0){          /*???????*/
		SwitchCommand();
		__cmd_QUERYFARE_Handler();
	} else if(strncmp(tmp, "01", 2) == 0){   /*?????????*/
		SwitchCommand();
		__cmd_QUERYFLOW_Handler();
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
	char tmp[3], mold;
	char *ret;
	char verify = 0;
	
	const static ProtocolHandleMap map[] = {  
		{GATEPARAM,      HandleGatewayParam},     /*0x01; ���ز�������*/           ///
		{LIGHTPARAM,     HandleLightParam},       /*0x02; �Ʋ�������*/             /// 
		{STRATEGY,       HandleStrategy},         /*0x03; ��������*/               ///
		{DIMMING,        HandleLightDimmer},      /*0x04; �Ƶ������*/
		{LAMPSWITCH,     HandleLightOnOff},       /*0x05; �ƿ��ؿ���*/
		{READDATA,       HandleReadBSNData},      /*0x06; ������������*/
		{LOOPCONTROL,    HandleGWloopControl},    /*0x07; ���ػ�·����*/           ///
		{DATAQUERY,      HandleGWDataQuery},      /*0x08; �������ݲ�ѯ*/           
		{TIMEQUERY,      HandleGWTurnTimeQuery},  /*0x09; ���ؿ��ص�ʱ���ѯ*/     ///
		{AUTOWORK,       HandleLightAuto},        /*0x0A; ���Զ�����*/
		{TIMEADJUST,     HandleAdjustTime},       /*0x0B; Уʱ*/                   ///          
		{VERSIONQUERY,   HandleGWVersQuery},      /*0x0C; ����������汾��*/       ///
		{ELECTRICGATHER, HandleEGVersQuery},      /*0x0E; ������ɼ�����汾��*/
		{ADDRESSQUERY,   HandleGWAddrQuery},      /*0x11; ���ص�ַ��ѯ*/           ///
		{SETSERVERIP,    HandleSetGWServ},        /*0x14; ��������Ŀ�������IP*/   ///
		{GATEUPGRADE,    HandleGWUpgrade},        /*0x15; ����Զ������*/
		{GATHERUPGRADE,  HandleEGUpgrade},        /*0x1E; �����ɼ�ģ��Զ������*/
		{BALLASTUPGRADE, HandleBSNUpgrade},       /*0x2A; ������Զ������*/
		{RSSIVALUE,      HandleRSSIQuery},        /*0x17; GSMģ���ź�ǿ�Ȳ�ѯ*/
		{RESTART,        HandleRestart},          /*0x3F; �豸��λ*/               ///  
	};

	ret = p;
	
	for (i = 0; i < (strlen(p) - 3); ++i) {
		verify ^= *ret++;
	}
	
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

