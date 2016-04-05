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

static xSemaphoreHandle __TransSemaphore; 

static xSemaphoreHandle __ElecSemaphore;

static xSemaphoreHandle __BSNSemaphore;

void SemaInit(void) {
	if (__TransSemaphore == NULL) {
		vSemaphoreCreateBinary(__TransSemaphore);
	}
	
	if (__ElecSemaphore == NULL) {
		vSemaphoreCreateBinary(__ElecSemaphore);
	}
	
	if (__BSNSemaphore == NULL) {
		vSemaphoreCreateBinary(__BSNSemaphore);
	}
}

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
	SETPARAMLIMIT = 0x21,   /*���ù�ǿ�ȶκ�ʱ��λ��ֵ����*/
	STRATEGYDOWN = 0x22,    /*��������ز�������*/
	GATEUPGRADE = 0x37,     /*����Զ������*/
	TIMEADJUST = 0x42,      /*Уʱ*/
	LUXVALUE = 0x43,        /*���յ���ǿ��ֵ*/
	/*0x40, �����������������ָ��*/
	/*0x45, �����������ʱ������ָ��*/
	/*0x44, �������ӵ�ַ��һ��ʱ���͵�ָ��*/
	RESTART = 0x3F,         /*�豸��λ*/
	RETAIN,                 /*����*/
} GatewayType;

typedef struct{
	char BeforClose[2];     /*�ص�ǰƫ��*/
	char AfterClose[2];     /*�صƺ�ƫ��*/
	char BeforOpen[2];      /*����ǰƫ��*/
	char AfterOpen[2];      /*���ƺ�ƫ��*/
	char LateNightStart[4]; /*��ҹ��ʼʱ��*/
	char LateNightEnd[4];   /*��ҹ����ʱ��*/
}TimeZoneDog;             /*ʱ�����򻮷ֵ�*/

typedef struct{
	char HardLightDot[6];      /*ǿ����޵�*/
	char MiddleLightDot[6];    /*�й���޵�*/
	char WeakLightDot[6];      /*������޵�*/
}LuxZoneDog;              /*��ǿ�����򻮷ֵ�*/


typedef struct{
	char MainACT;           /*��������*/
	char MainSpaceNUM;      /*�������������Ŀ*/
	char AssistantACT;      /*��������*/
	char AssistSpaceNUM;    /*�������������Ŀ*/
	char SpotLightACT;      /*Ͷ�⶯��*/
	char SpotSpaceNUM;      /*Ͷ����������Ŀ*/
}SectionStrategy;         /*����ڶβ���*/

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

static unsigned char *BSNRespondBuf[240];

void BsnFuntion(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret;
	unsigned char hexTable[] = "0123456789ABCDEF";
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 9 + len + 3 + 2;
	
	ret = (unsigned char *)BSNRespondBuf;
	
	
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
}

unsigned char *DataSendToBSN(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	if (xSemaphoreTake(__BSNSemaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		
		BsnFuntion(address, type, msg, size);
		xSemaphoreGive(__BSNSemaphore);
		
		return (unsigned char *)BSNRespondBuf;
	}
}

static unsigned char RespondBuf[240];

void NoRenntry(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	unsigned char hexTable[] = "0123456789ABCDEF";
	
	*size = 15 + len + 3;
	if(type[1] > '9'){
		i = (unsigned char)(type[0] << 4) | (type[1] - 'A' + 10);
	} else {
		i = (unsigned char)(type[0] << 4) | (type[1] & 0x0f);
	}
	i = i | 0x80;
	{
		ProtocolHead *h = (ProtocolHead *)RespondBuf;
		h->header = 0x02;	
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = hexTable[i >> 4];
		h->contr[1] = hexTable[i & 0x0F];
		h->lenth[0] = hexTable[(len >> 4) & 0x0F];
		h->lenth[1] = hexTable[len & 0x0F];
	}

	if (msg != NULL) {
		strcpy((char *)(RespondBuf + 15), msg);
	}
	
	p = RespondBuf;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
}



unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	if (xSemaphoreTake(__TransSemaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		
		NoRenntry(address, type, msg, size);
		xSemaphoreGive(__TransSemaphore);
		
		return RespondBuf;
	}
}

static unsigned char ElecRespondBuf[240];

void NoRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	int i;
	unsigned int verify = 0;
	unsigned char *p;
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	unsigned char hexTable[] = "0123456789ABCDEF";
	
	*size = 15 + len + 3;
	i = (unsigned char)(type[0] << 4) + (type[1] & 0x0f);
	
	{
		ProtocolHead *h = (ProtocolHead *)ElecRespondBuf;
		h->header = 0x02;
		strcpy((char *)h->addr, (const char *)address);
		h->contr[0] = hexTable[i >> 4];
		h->contr[1] = hexTable[i & 0x0F];
		h->lenth[0] = hexTable[(len >> 4) & 0x0F];
		h->lenth[1] = hexTable[len & 0x0F];
	}

	if (msg != NULL) {
		strcpy((char *)(ElecRespondBuf + 15), msg);
	}
	
	p = ElecRespondBuf;
	for (i = 0; i < (len + 15); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
}

unsigned char *ProtocolToElec(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
	if (xSemaphoreTake(__ElecSemaphore, configTICK_RATE_HZ * 5) == pdTRUE) {
		
		NoRespond(address, type, msg, size);
		xSemaphoreGive(__ElecSemaphore);
		
		return ElecRespondBuf;
	}
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
}

static unsigned short MaxZigbeeAdress = 0;       /*����������ZigBee��ַ*/
static unsigned short MaxZigBeeNumb = 0;         /*�����½ڵ����Ŀ*/

static void __ParamWriteToFlash(const char *p){
	unsigned char msg[5];
	unsigned short len;
	DateTime dateTime;
	uint32_t second;	
	Lightparam g, s;
	
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
		
		if(len > 1000){
			MaxZigBeeNumb++;
			return;
		}
		
		if(MaxZigbeeAdress < len){
			MaxZigbeeAdress = len;		
		}
		
		NorFlashRead(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (short *)&s, (sizeof(Lightparam) + 1) / 2);	
		if(s.AddrOfZigbee[0] == 0xFF)
			MaxZigBeeNumb++;
					
		NorFlashWrite(NORFLASH_BALLAST_BASE + len * NORFLASH_SECTOR_SIZE, (const short *)&g, (sizeof(Lightparam) + 1) / 2);	
}

static void HandleLightParam(ProtocolHead *head, const char *p) {
	unsigned short len, i = 0, j, lenth;
	unsigned char size;
	unsigned char *buf, msg[48], tmp[5];
	unsigned short temp[3] = {0};
	
	
	NorFlashRead(NORFLASH_LIGHT_NUMBER, (short *)temp, 2);
	if(temp[0] > 0x1000)
		temp[0] = 0;
	if(temp[1] > 0x1000)
		temp[1] = 0;
	MaxZigBeeNumb = temp[0];
	MaxZigbeeAdress = temp[1];

	if(p[0] == '1'){          /*����һյ��*/
		
		len = (strlen(p) - 1) / 17;

		for(i = 0; i < len; i++) {
			__ParamWriteToFlash(&p[1 + i * 17]);
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4])); 
		}
		msg[i * 4] = p[0];
		
	} else if (p[0] == '2'){   /*ɾ��һյ��*/
		Lightparam g;
		
		len = (strlen(p) - 3) / 17;

		for(i = 0; i < len; i++) {
			sscanf(&p[1 + i * 17], "%4s", &(msg[i * 4]));
			sscanf(&p[1 + i * 17], "%4s", tmp);
		
			lenth = atoi((const char *)tmp);
			
			if(lenth < 1000)
				NorFlashEraseParam(NORFLASH_BALLAST_BASE + lenth * NORFLASH_SECTOR_SIZE);
			
			if(MaxZigBeeNumb > 0)
				MaxZigBeeNumb--;
			
			for(j = MaxZigBeeNumb; j <= MaxZigbeeAdress; j++){
				NorFlashRead(NORFLASH_BALLAST_BASE + j * NORFLASH_SECTOR_SIZE, (short *)&g, (sizeof(Lightparam) + 1) / 2);
				
			if(g.AddrOfZigbee[0] == 0xFF)
				continue;
			
			sscanf(g.AddrOfZigbee, "%4s", tmp);
			lenth = atoi((const char *)tmp);
			
			if(lenth < 1000)			
				MaxZigbeeAdress = lenth;
			}						
		}
		msg[i * 4] = p[0];
		
	} else if (p[0] == '4'){	
		char j;
		
		MaxZigBeeNumb = 0;
		MaxZigbeeAdress = 0;
		
		for(j = 0; j < 60; j++){
			NorFlashEraseBlock(NORFLAH_ERASE_BLOCK_BASE + j * NORFLASH_BLOCK_SIZE);
		}
	
		msg[0] = '0';
		msg[1] = '0';
		msg[2] = '0';
		msg[3] = '0';
		i++;
		msg[i * 4] = p[0];
	}
		
	msg[i * 4 + 1] = 0;
	
	temp[0] =  MaxZigBeeNumb;
	temp[1] =  MaxZigbeeAdress;
	NorFlashWrite(NORFLASH_LIGHT_NUMBER, (const short *)temp, 2);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
}

static unsigned short SeekAddr[600];              /*�洢��Ҫ��ѯZigBee��ַ����*/

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
	CurcuitContrInit();
	memset(SeekAddr, 0, 600);
	
	memset(__msg.ArrayAddr, 0, 600);
	if (__ProSemaphore == NULL) {
		vSemaphoreCreateBinary(__ProSemaphore);
	}
	SemaInit();
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
	GUIDESECTION = 1,   /*������*/
	ENTRANCE,           /*��ڶ�*/
	TRANSITION_I,       /*��ڹ��ɶ�*/
	INTERMEDIATE,       /*�м��*/
	TRANSITON_II,       /*���ڹ��ɶ�*/
	EXITSECTION,        /*���ڶ�*/	
}PartType;            /*������ζ���*/

#define NoChoice     0           /*����û��ѡ��*/

unsigned short *RespondAddr(void){
	return SeekAddr;
}

static char CommandFlag = 0;

void __DataFlagJudge(const char *p){
	int i = 0, j, len = 0, m, n;
	unsigned char tmp[5], *ret, *msg;
	Lightparam k;
	
//	msg = DataFalgQueryAndChange(2, 0, 1);
//	if(*msg == 1){
//		ret = (unsigned char *)p + 4;
//	} else if(*msg == 3) {
//		ret = (unsigned char *)p + 5;
//	} else if(*msg == 2) {
//		ret = (unsigned char *)p + 6;
//	}
	
	if(CommandFlag == 1){
		ret = (unsigned char *)p + 4;
	} else if(CommandFlag == 3) {
		ret = (unsigned char *)p + 5;
	} else if(CommandFlag == 2) {
		ret = (unsigned char *)p + 6;
	} else {
		return;
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
	
	for(i = 0; i < 1000; i++){
		SeekAddr[i] = __msg.ArrayAddr[i];
		if(__msg.ArrayAddr[i] == 0)
			break;
	}
	
	CommandFlag = 0;
} 

static unsigned char DataMessage[32];

unsigned char *__datamessage(void){
	return DataMessage;
}



static void HandleLightDimmer(ProtocolHead *head, const char *p){
	unsigned char *buf, msg[8], *ret, size;
	
	ret = DataFalgQueryAndChange(5, 0, 1);
	while(*ret != 0){
		return;
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	CommandFlag = 2;
//	DataFalgQueryAndChange(2, 2, 0);
	sscanf(p, "%6s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	
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
		return;
	}
	
	DataFalgQueryAndChange(5, 1, 0);
	
	CommandFlag = 3;
//	DataFalgQueryAndChange(2, 3, 0);
	sscanf(p, "%5s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	
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
	
//	buf = DataFalgQueryAndChange(5, 0, 1);
//	while(*buf != 0){
//		return;
//	}
//	
	DataFalgQueryAndChange(5, 1, 0);
//	
	CommandFlag = 1;
//	DataFalgQueryAndChange(2, 1, 0);
	sscanf(p, "%4s", msg);
	
	buf = ProtocolRespond(head->addr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	
	__DataFlagJudge(p);
}

static void HandleGWDataQuery(ProtocolHead *head, const char *p) {     /*���ػ�·���ݲ�ѯ*/
	unsigned char *buf,size, loop;
	
//	buf = DataFalgQueryAndChange(5, 0, 1);
//	while(*buf != 0){
//		return;
//	}
//	
//	DataFalgQueryAndChange(2, 4, 0);
//	DataFalgQueryAndChange(6, p[0], 0);
// // ElecTaskSendData((const char *)(p - sizeof(ProtocolHead)), (sizeof(ProtocolHead) + strlen(p)));

//	DataFalgQueryAndChange(5, 1, 0);
	loop = p[0];

	buf = ProtocolToElec(head->addr, (unsigned char *)"08", (const char *)&loop, &size);
	ElecTaskSendData((const char *)buf, size);	
}

static void HandleLightAuto(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
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
	
//	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
//  GsmTaskSendTcpData((const char *)buf, size);
}

static void HandleGWVersQuery(ProtocolHead *head, const char *p) {      /*����������汾��*/
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, __TARGET_STRING__, &size);
  GsmTaskSendTcpData((const char *)buf, size);
}

static int HardLight = 10000;    /*ǿ��ֵ*/
static int MiddleLight = 500;    /*�й�ֵ*/
static int WeakLight = 50;       /*����ֵ*/

#define OnOffLight     1     /*���ص�ʱ��*/
#define HaveSun        2     /*����ʱ��*/
#define HaveMoon       3     /*ҹ��ʱ��*/
#define LateNight      4     /*��ҹʱ��*/     /*˳�򲻿ɵߵ�*/

#define GoodSight      1        /*����ʮ�ֺ�*/ /*ǿ����*/
#define CommonSight    2        /*��������*/   /*�й���*/
#define HardSight      3        /*�ܼ��Ⱥܵ�*/ /*������*/
#define LoseSight      4        /*������*/     /*�޹���*/   /*˳�򲻿ɵߵ�*/

static short BeforCloseOffset = 30 * 60;            /*�ص�ǰƫ��*/  
static short AfterCloseOffset = 30 * 60;            /*�صƺ�ƫ��*/  
static short BeforOpenOffset = 30 * 60;             /*����ǰƫ��*/  
static short AfterOpenOffset = 30 * 60;             /*���ƺ�ƫ��*/  
static int DeepNightStart = (22 * 60 * 60);   /*��ҹ��ʼʱ��*/  
static int DeepNightEnd = (5 * 60 * 60);      /*��ҹ����ʱ��*/  

static void HandleSetParamDog(ProtocolHead *head, const char *p){         /*���ù�ǿ�������ʱ�����򻮷ֵ�*/
	unsigned char *buf, size, tmp[8];
	LuxZoneDog k;
	TimeZoneDog g;
	
	sscanf(p, "%6s", tmp);
	HardLight = atoi((const char *)tmp);
	if(HardLight < 100000)
		sscanf(p, "%6s", k.HardLightDot);          /*����ǿ���*/
	
	sscanf(p, "%*6s%6s", tmp);
	MiddleLight = atoi((const char *)tmp);    
	if(MiddleLight < 10000)
		sscanf(p, "%*6s%6s", k.MiddleLightDot);    /*�����й��*/
	
	sscanf(p, "%*12s%6s", tmp);
	WeakLight = atoi((const char *)tmp);
	if(WeakLight < 1000)
		sscanf(p, "%*12s%6s", k.WeakLightDot);     /*���������*/
	
	NorFlashWrite(PARAM_LUX_DOG_OFFSET, (short *)&k, (sizeof(LuxZoneDog) + 1) / 2);   /*�洢��ǿ���򻮷ֵ����*/
	
	sscanf(p, "%*18s%2s", g.BeforClose);      /*���ùص�ǰƫ��*/
	sscanf(p, "%*20s%2s", g.AfterClose);      /*���ùصƺ�ƫ��*/
	sscanf(p, "%*22s%2s", g.BeforOpen);       /*���ÿ���ǰƫ��*/
	sscanf(p, "%*24s%2s", g.AfterOpen);       /*���ÿ��ƺ�ƫ��*/
	sscanf(p, "%*26s%4s", g.LateNightStart);  /*������ҹ��ʼʱ��*/
	sscanf(p, "%*30s%4s", g.LateNightEnd);    /*������ҹ����ʱ��*/
	
	NorFlashWrite(PARAM_TIME_DOG_OFFSET, (short *)&g, (sizeof(TimeZoneDog) + 1) / 2);  /*�洢ʱ��λ��ֵ����*/
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
}

static uint32_t RealAddr = 0;

uint32_t FragOffset(char sec, char tim, char lux){                        /*���ݻ�·��ʱ���򣬹�ǿ���ҳ����Դ��λ��*/    
	uint32_t SecStart, SecOffset;
	
	SecStart = (sec - 1) * SECTION_SPACE_SIZE + STRATEGY_GUIDE_ONOFF_DAZZLING;   /*���ڶε���ʼλ��*/	
	SecOffset = (4 * (tim - 1) + (lux - 1)) *NORFLASH_SECTOR_SIZE;               /*����ƫ�Ƶ�ַ*/
	
	RealAddr = SecStart + SecOffset;
	return RealAddr;
}

static char StrategeFlag = 0;              /*�������ػ��߸ı��־��*/

char StrategeChange(void){
	return StrategeFlag;
}

void SetFlagBit(void){
	StrategeFlag = 0;
}

static char UpdataTimes = 0;

static char DownEnd = 0;

static void HandleStrategy(ProtocolHead *head, const char *p){          /*��������*/
	unsigned char *buf, size, tmp[8], loop, Tim_Zone, Lux_Zone;
	uint32_t StoreSpace = 0;
	short i, a, b;
	char x, y, z;
	SectionStrategy s;
	
	sscanf(p, "%*6s%2s", tmp);               /*��������ѡ��*/
	if(tmp[0] != '0'){
		s.MainACT = tmp[0] - '0';
		if(tmp[0] == '3')
			s.MainSpaceNUM = tmp[1] - '0';
	}	
	
	sscanf(p, "%*8s%2s", tmp);               /*��������ѡ��*/
	if(tmp[0] != '0'){
		s.AssistantACT = tmp[0] - '0';
		if(tmp[0] == '3')
			s.AssistSpaceNUM= tmp[1] - '0';
	}
	
	sscanf(p, "%*10s%2s", tmp);              /*Ͷ�⶯��ѡ��*/
	if(tmp[0] != '0'){
		s.SpotLightACT = tmp[0] - '0';
		if(tmp[0] == '3')
			s.SpotSpaceNUM = tmp[1] - '0';
	}
	
	sscanf(p, "%2s", tmp);                   /*��·ѡ��*/
	loop = strtol((const char *)tmp, NULL, 16);
	
	sscanf(p, "%*2s%2s", tmp);               /*ʱ���ѡ��*/
	Tim_Zone = strtol((const char *)tmp, NULL, 16);
	
	sscanf(p, "%*4s%2s", tmp);               /*��ǿ��ѡ��*/
	Lux_Zone = strtol((const char *)tmp, NULL, 16);
	
	for(i = 0x1, x = 1; x < 6; (i = i << 1), x++){          /*�ж��м�����·*/ /*���6����·*/
		if(!(i & loop))
			continue;
		for(a = 0x1, y = 1; y < 5; (a = a << 1), y++){    /*�ж��м���ʱ����*/   /*���4��ʱ����*/
			if(!(a & Tim_Zone))
				continue;			
			for(b = 0x1, z = 1; z < 5; (b = b << 1), z++){  /*�ж��м�����ǿ��*/   /*���4����ǿ��*/
				if(!(b & Lux_Zone))
					continue;			
				StoreSpace = FragOffset(x, y, z);             /*���Դ��λ��*/
				NorFlashWrite(StoreSpace, (short *)&s, (sizeof(SectionStrategy) + 1) / 2);
			}
		}
	}
	
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	
	UpdataTimes++;
	if(UpdataTimes == 6){
		DownEnd= 1;
		UpdataTimes = 0;
	}
}

static void HandleEGVersQuery(ProtocolHead *head, const char *p) {
	unsigned char *buf, size;
	
	buf = ProtocolRespond((unsigned char *)"9999999999", head->contr, NULL, &size);
	ElecTaskSendData((const char *)buf, size);
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

static short LightAddr[600];            /*��Ҫ�������Ƶ�ַ����*/
static short LightCount = 0;            /*��Ҫ�������Ƽ���*/

const short *LightZigbAddr(void){
	return LightAddr;
}

void ActionControl(short addr, short pole, char act, char num){                   /*��������Ͷ���Կ��Ƶ�����ַ*/
	unsigned char tmp[5], *buf, size, ret[9];
	char lightFlag = 0;
	DateTime dateTime;
	uint32_t second;
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);
	
	if(dateTime.hour < 12)
		pole += 1; 
	if(act == 1){		
		lightFlag = 1;
	} else if(act == 3){
		if((pole % (num + 1)) == 1)        /*���Ƶ���*/
			lightFlag = 1;
	}
	
	if(lightFlag){
		LightAddr[LightCount++] = addr;
	}	
}

void NoneStrategy(char loop, char tim, short addr, char param){      /*û�����ò��ԵĲ���*/
	if((loop- '0') > 1){                  /*��������*/
		if((tim == 1) || (tim == 2)){       /*����*/
			if(param == 0){                   /*����*/ 
				LightAddr[LightCount++] = addr;
			}
		} else if((tim == 3) || (tim == 4)){/*ҹ��*/
			if(param == 1){                   /*����*/ 
				LightAddr[LightCount++] = addr;
			}
		}
	} else {
		if((tim == 3) || (tim == 4)){       /*ҹ��*/
			if(param == 0){                   /*����*/ 
				LightAddr[LightCount++] = addr;
			}
		} 	
	}
}

void __handleLux(char tim, char lux){            /*���ݻ�·����ǿ��ʱ����ʵ�в���*/
	short i, tmp[3], Zigb, Pole;
	Lightparam k;
	SectionStrategy s;
	uint32_t StartAddr = 0;
	char buf[5], LightPara;
	
	NorFlashRead(NORFLASH_LIGHT_NUMBER, (short *)tmp, 2);      /*ȡ�����ZIGBEE��ַ*/
	for(i = 1; i <= tmp[1]; i++){		
		NorFlashRead((NORFLASH_BALLAST_BASE + i * NORFLASH_SECTOR_SIZE), (short *)&k, (sizeof(Lightparam) + 1) / 2);
		if((k.Loop > '8') || (k.Loop < '0'))
			continue;
		
		StartAddr = FragOffset(k.Loop - '0', tim, lux);
		
		sscanf((const char *)k.AddrOfZigbee, "%4s", buf);
		Zigb = atoi((const char *)buf);
		
		sscanf((const char *)k.LightPole, "%4s", buf);
		Pole = atoi((const char *)buf);
		
		sscanf((const char*)k.Attribute, "%1s", buf);
		LightPara = atoi((const char *)buf);
		
		NorFlashRead(StartAddr, (short *)&s, (sizeof(SectionStrategy) + 1) / 2);
		
		if((k.Attribute[0] == '0') && (s.MainACT < '9')){               /*����*/
			ActionControl(Zigb, Pole, s.MainACT, s.MainSpaceNUM);
			continue;
		} else if((k.Attribute[0] == '1' )&& (s.AssistantACT < '9')){   /*����*/
			ActionControl(Zigb, Pole, s.AssistantACT, s.AssistSpaceNUM);
			continue;
		} else if((k.Attribute[0] > '1' )&& (s.SpotLightACT < '9')){    /*Ͷ��*/
			ActionControl(Zigb, Pole, s.SpotLightACT, s.SpotSpaceNUM);
			continue;
		}
		NoneStrategy(k.Loop, tim, Zigb, LightPara);
	}
	
}

typedef struct{
	char count;               /*�յ��Ĺ��ն�ֵ����*/
	char TimeArea;            /*��ǰ���ڵ�ʱ��*/
	char LuxArea;             /*��ǰ���ڵĹ�ǿ��*/
	int LastLux;              /*�ϴθı��ǿ��ʱ�յ�12�ι��ն�ֵ��ƽ��ֵ*/
	int NowLux;               /*��ǰ1�����ڹ��նȵ�ƽ��ֵ*/
	int ArrayOfLuxValue[12];   /*�������12�ι��նȵ�����*/	
}StoreParam;                                     

static char LastTimArea = 4;  /*ǰһ����ʱ������*/
static char LasrLuxArea = 1;  /*ǰһ���ӹ�ǿ����*/

static StoreParam __Luxparam = {0, 2, 2, 0, 0, 0};

static void DivisiveLightArea(int lux){     /*���ݵ�ǰ���նȣ���������������*/
	char state, tmp[8];
	int val;
	LuxZoneDog k;
	
	NorFlashRead(PARAM_LUX_DOG_OFFSET, (short *)&k, (sizeof(LuxZoneDog) + 1) / 2); 
	if(k.HardLightDot[0] < 0x41) {
		sscanf(k.HardLightDot, "%6s", tmp);
		HardLight = atoi((const char *)tmp);	
	}
	
	if(k.MiddleLightDot[0] < 0x41){
		sscanf(k.MiddleLightDot, "%6s", tmp);
		MiddleLight = atoi((const char *)tmp);	
	}
	
	if(k.WeakLightDot[0] < 0x41){
		sscanf(k.WeakLightDot, "%6s", tmp);
		WeakLight = atoi((const char *)tmp);	
	}
		
	if(lux >= HardLight){
		state = GoodSight;
	} else if ((lux >= MiddleLight) && (lux < HardLight)){
		state = CommonSight;
	} else if((lux >= WeakLight) && (lux < MiddleLight)){
		state = HardSight;
	} else{
		state = LoseSight;
	}
	
	if(state != __Luxparam.LuxArea)
		val = abs(lux - __Luxparam.LastLux);
	
	if(__Luxparam.LastLux > 300){
		if((__Luxparam.LastLux / val) < 4)
			__Luxparam.LuxArea = state;
	} else {
		if((__Luxparam.LastLux / val) < 2)
			__Luxparam.LuxArea = state;
		
	}
		
}

extern unsigned char *DayToSunshine(void);

extern unsigned char *DayToNight(void); 

static void DivideTimeArea(DateTime time){   /*���ݵ�ǰʱ�䣬����������ʱ��*/ 
	int NowTime, DayTime, DarkTime;
	unsigned char *buf, tmp[6], hour, minute;
	TimeZoneDog k;
	
	NowTime = time.hour * 3600 + time.minute * 60 + time.second;    /*��ǰʱ��*/
	
	buf = DayToSunshine();
	DarkTime = buf[0] * 3600 + buf[1] * 60 + buf[2];                  /*����ʱ��*/
	
	buf = DayToNight();
	DayTime = buf[0] * 3600 + buf[1] * 60 + buf[2];                /*�ص�ʱ��*/    

	NorFlashRead(PARAM_TIME_DOG_OFFSET, (short *)&k, (sizeof(TimeZoneDog) + 1) / 2); 
	
	if(k.AfterClose[0] < 0x41){
		sscanf(k.AfterClose, "%2s", tmp);
		AfterCloseOffset = atoi((const char *)tmp) * 60;
	}
	
	if(k.AfterOpen[0] < 0x41){
		sscanf(k.AfterOpen, "%2s", tmp);
		AfterOpenOffset = atoi((const char *)tmp) * 60;
	}
	
	if(k.BeforClose[0] < 0x41){
		sscanf(k.BeforClose, "%2s", tmp);
		BeforCloseOffset = atoi((const char *)tmp) * 60;
	}
	
	if(k.BeforOpen[0] < 0x41){
		sscanf(k.BeforOpen, "%2s", tmp);
		BeforOpenOffset = atoi((const char *)tmp) * 60;
	}
	
	if(k.LateNightEnd[0] < 0x41){
		sscanf(k.LateNightEnd, "%2s", tmp);
		hour = atoi((const char *)tmp);
		sscanf(k.LateNightEnd, "%*2s%2s", tmp);
		minute = atoi((const char *)tmp);
		
		DeepNightEnd = hour * 3600 + minute * 60;
	}
	
	if(k.LateNightStart[0] < 0x41){
		sscanf(k.LateNightStart, "%2s", tmp);
		hour = atoi((const char *)tmp);
		sscanf(k.LateNightStart, "%*2s%2s", tmp);
		minute = atoi((const char *)tmp);
		
		DeepNightStart = hour * 3600 + minute * 60;
	}
	
	if((NowTime >= (DayTime + AfterOpenOffset )) && (NowTime <= (DarkTime - BeforOpenOffset ))){  /*�صƺ�ƫ�ƣ�������ǰƫ��*/
		__Luxparam.TimeArea = HaveSun;	                                  /*����Ϊ����ʱ��*/
	} else if((NowTime >= DeepNightStart) && (NowTime <= (DayTime - BeforCloseOffset)) && (NowTime < DeepNightEnd)){ /*��ҹ��ʼʱ�䵽�ڶ��쿪��ƫ��ǰ������ҹ����*/
		__Luxparam.TimeArea = LateNight;                                  /*����Ϊ�������˵���ҹʱ��*/
	} else if(((NowTime > (DayTime - BeforCloseOffset)) && (NowTime < (DayTime + AfterCloseOffset))) || 
						((NowTime < (DarkTime + AfterOpenOffset)) && (NowTime > (DarkTime - BeforOpenOffset)))){  /*����ǰƫ�Ƶ����ƺ�ƫ�ƣ����߹ص�ǰƫ�Ƶ��صƺ�ƫ��*/
		__Luxparam.TimeArea	= OnOffLight;				                           /*����Ϊ���ص�ʱ�䷶Χ��*/
	} else {
		__Luxparam.TimeArea	= HaveMoon;                                    /*����ʱ�䶨��Ϊ���ǻ�δ��Ϣ��ҹ��ʱ���*/
	}
}

void bubble(int *a,int n){     /*�����С��������*/ 
	int i,j,temp;  
	for(i=0;i<n-1;i++){  
		for(j=i+1;j<n;j++){ 
			if(a[i]>a[j]) { 
				temp=a[i]; 
				a[i]=a[j]; 
				a[j]=temp; 
			}
		}
	}		
} 

void BegAverage(int *p){
	char i;
	int sum = 0;
	
	bubble(p, 12);
	for(i = 2; i < 10; i++){
		sum += p[i];
	}
	
	__Luxparam.NowLux = sum / 10;
}

void CalulateLightAddr(void){

	memset(LightAddr, 0, 600);
	__handleLux(__Luxparam.TimeArea, __Luxparam.LuxArea);	
	
}

 void HandleLuxGather(ProtocolHead *head, const char *p) {
	char msg[10]; 
	unsigned char *buf, size;
	int LuxValue, second;
	DateTime dateTime;
	
  sscanf(p, "%8s", msg);
	LuxValue = atoi((const char *)msg);
	
	__Luxparam.ArrayOfLuxValue[__Luxparam.count] = LuxValue;
	__Luxparam.count++;
	
	if(__Luxparam.count > 11){
		second = RtcGetTime();
		SecondToDateTime(&dateTime, second);
		
		DivideTimeArea(dateTime);
		
		BegAverage(__Luxparam.ArrayOfLuxValue);
		DivisiveLightArea(__Luxparam.NowLux);
		
		if((LastTimArea != __Luxparam.TimeArea) || (LasrLuxArea != __Luxparam.LuxArea) || (DownEnd == 1)){
			LightCount = 0;
			memset(LightAddr, 0, 600);
			__handleLux(__Luxparam.TimeArea, __Luxparam.LuxArea);	
			
			StrategeFlag = 1;
			DownEnd = 0;
		}
		
		__Luxparam.count = 0;
		
		LastTimArea = __Luxparam.TimeArea;
		LasrLuxArea = __Luxparam.LuxArea;
	}
	
//	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
//  GsmTaskSendTcpData((const char *)buf, size);
}

static void HandleRestart(ProtocolHead *head, const char *p){            /*�豸��λ*/
	unsigned char *buf, msg[11], size;
	
	if((p[0] == '1')){
		NVIC_SystemReset();
	} else if(p[0] == '3'){
		sscanf(p, "%10s", msg);
		buf = ProtocolRespond("9999999999", head->contr, (const char *)msg, &size);
		ElecTaskSendData((const char *)buf, size);
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
		{SETPARAMLIMIT,  HandleSetParamDog},      /*0x21; ���ù�ǿ�������ʱ���򻮷ֵ����*/
		{STRATEGYDOWN,   HandleStrategy},         /*0x22; ��������*/
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

