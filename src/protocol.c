#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "sms.h"
#include "norflash.h"
#include "zklib.h"
#include "libupdater.h"
#include "norflash.h"
#include "second_datetime.h"
#include "rtc.h"
#include "version.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_flash.h"
#include "time.h"
#include "semphr.h"
#include "inside_flash.h"
#include "transfer.h"

#define Download_Store_Addr  (0x08040000)             //�����ļ��洢��ַ

extern void STMFLASH_Visit(uint32_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead);

typedef enum{
	ACKERROR = 0,           /*��վӦ���쳣*/
	TIMEADJUST = 0x0B,      /*Уʱ*/
	VERSIONQUERY = 0x0C,    /*��������汾�Ų�ѯ*/ 
	SETSERVERIP = 0x14,     /*��������Ŀ�������IP*/
	GATEUPGRADE = 0x16,     /*���մ���������Զ������*/
	RSSIVALUE = 0x17,       /*GSM�ź�ǿ�Ȳ�ѯ*/
	TUNNELUPGRADE = 0x20,   /*�������������*/
	CALLBALANCE = 0x21,     /*�ֻ����Ѳ�ѯ*/
	FLOWBALANCE = 0x22,     /*�ֻ�������ѯ*/
	LUXQUERY = 0x23,        /*���նȲ�ѯ*/
	RETAIN,                 /*����*/
} GatewayType;


typedef enum{
	REQUESTPACK = 0x40,     /*�������������������*/
	UPGRATEISOK = 0x41,     /*�����������������*/
	TIMECHECK = 0x42,       /*�˶�ʱ��*/
	LIGHTLUX = 0x43,        /*�������նȻظ�*/
	GATEWAYID = 0x44,       /*���ص�ַ�·�*/
	
	
	NONE,
}InternalType;

typedef struct {
	unsigned char BCC[2];
	unsigned char x03;
} ProtocolTail;

static void ProtocolDestroyMessage(const char *message) {
	vPortFree((void *)message);
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

unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size) {
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
	i = i | 0x80;
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


extern bool __GPRSmodleReset(void);

static void HandleSetGWServ(ProtocolHead *head, const char *p) {      /*��������Ŀ�������IP*/
	unsigned char *buf, size, tmp[6];
	GMSParameter g;
	
	sscanf(p, "%[^,]", g.serverIP);
	sscanf(p, "%*[^,]%*c%[^;]", tmp);
	
	g.serverPORT = atoi((const char *)tmp);
	
	NorFlashWrite(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	while(!__GPRSmodleReset());	
}

static void HandleTunnelUpgrate(ProtocolHead *head, const char *p) {           //��������أ�FTPԶ������
	const char *remoteFile = "STM32.PAK";
	unsigned short port = 21;
	FirmwareUpdaterMark *mark;
	char host[16], tmp[5];
	int len;
	
	sscanf((const char *)head->lenth, "%2s", tmp);
  len = strtol((const char *)tmp, NULL, 16);
	sprintf(tmp, "%%%ds", (len - 6));
	sscanf((p + 6), tmp, host);
	mark = pvPortMalloc(sizeof(*mark));
	if (mark == NULL) {
		return;
	}

	if (FirmwareUpdateSetMark(mark, host, port, remoteFile, 2)){
		NVIC_SystemReset();
	}
	vPortFree(mark);
}

static void HandleGWUpgrade(ProtocolHead *head, const char *p){             //���մ�����DTU��FTPԶ������
	const char *remoteFile = "STM32.PAK";
	unsigned short port = 21;
	FirmwareUpdaterMark *mark;
	char host[16], tmp[5];
	int len;
	
	sscanf((const char *)head->lenth, "%2s", tmp);
  len = strtol((const char *)tmp, NULL, 16);
	sprintf(tmp, "%%%ds", (len - 6));
	sscanf((p + 6), tmp, host);
	mark = pvPortMalloc(sizeof(*mark));
	if (mark == NULL) {
		return;
	}

	if (FirmwareUpdateSetMark(mark, host, port, remoteFile, 1)){
		NVIC_SystemReset();
	}
	vPortFree(mark);
}

extern bool GsmTaskSendAtCommand(const char *atcmd);
extern void SwitchCommand(void);

static void HandleRSSIQuery(ProtocolHead *head, const char *p) {           //GSM�źŲ�ѯ
	SwitchCommand();	
	GsmTaskSendAtCommand("AT+CSQ\r");
}

static void HandleEGUpgrade(ProtocolHead *head, const char *p) {
	
}

extern void __cmd_QUERYFARE_Handler(void);
extern void __cmd_QUERYFLOW_Handler(void);

static void HandleCallBalance(ProtocolHead *head, const char *p) {
	SwitchCommand();
	__cmd_QUERYFARE_Handler();
}


static void HandleFlowBalance(ProtocolHead *head, const char *p) {
	SwitchCommand();
	__cmd_QUERYFLOW_Handler();
}

extern unsigned short GetLux(void);

static void HandleLuxQuery(ProtocolHead *head, const char *p) {
	unsigned char *buf, size, tmp[8];
	
	sprintf((char *)tmp, "%06d", GetLux());
	buf = ProtocolRespond(head->addr, head->contr, (const char *)tmp, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
}
typedef void (*ProtocolHandleFunction)(ProtocolHead *head, const char *p);

typedef struct {
	GatewayType type;
	ProtocolHandleFunction func;
} ProtocolHandleMap;


/*GW: gateway  ����*/
/*EG: electric quantity gather �����ɼ���*/
/*illuminance �� ���ն�*/
/*BSN: �Ƶ�������*/
void GPRSProtocolHandler(ProtocolHead *head, char *p) {
	int i;
	char tmp[3], mold;
	char *ret;
	char verify = 0;
	
	const static ProtocolHandleMap map[] = {  
		{TIMEADJUST,    HandleAdjustTime},       /*0x0B; Уʱ*/                   ///          
		{SETSERVERIP,   HandleSetGWServ},        /*0x14; ��������Ŀ�������IP*/   ///
		{GATEUPGRADE,   HandleGWUpgrade},        /*0x16; ���մ���������Զ������*/
		{RSSIVALUE,     HandleRSSIQuery},        /*0x17; GSMģ���ź�ǿ�Ȳ�ѯ*/
		{TUNNELUPGRADE, HandleTunnelUpgrate},    /*0x20; ����ڴ�������������*/
		{CALLBALANCE,   HandleCallBalance},      /*0x21; ��ѯ�ֻ�ʣ�໰��*/
		{FLOWBALANCE,   HandleFlowBalance},      /*0x22; ��ѯ�ֻ�ʹ������*/
		{LUXQUERY,      HandleLuxQuery},         /*0x23; ��ѯ���ն�*/
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
			return;
		}
	}	
}

/*����ĳ���������������նȲɼ������������ذ�֮�䴫�����ݵ�Э��*/

static void HandleUpdataPacket(ProtocolHead *head, const char *p) {
	unsigned char section, buf[16], temp[1024], i;
	short size;
	
	
	sscanf(p, "%2s", buf);
	section = atoi((const char *)buf);
	
	sscanf(p, "%*2s%4s", buf);
	size = atoi((const char *)buf);
	
	STMFLASH_Visit(Download_Store_Addr + (section - 1) * 1024, temp, size);
	
	section = size / 200;
	if(size % 200)
		section++;
		
	for(i = 0; i < section; i++){
		if(i != (section - 1))
			TransTaskSendData((const char *)&temp[i * 200], 200);
		else
			TransTaskSendData((const char *)&temp[i * 200], size % 200);
	}
}

extern void FirmwareUpdaterEraseMark(void);

static void HandleUpgrareOK(ProtocolHead *head, const char *p) {
	FirmwareUpdaterEraseMark();
}

static void HandleLUX(ProtocolHead *head, const char *p) {
	
}

static void HandleGWAddr(ProtocolHead *head, const char *p) {
	
}

static void HandleTimeFit(ProtocolHead *head, const char *p) {
	
}

typedef void (*InternalProtocolHandle)(ProtocolHead *head, const char *p);

typedef struct {
	InternalType type;
	InternalProtocolHandle func;
} InternalProtocolHandleMap;

void __handleInternalProtocol(ProtocolHead *head, char *p){
	int i, len;
	char tmp[3], mold, buf[16];
	char *ret;
	char verify = 0;
	
	const static InternalProtocolHandleMap map[] = {  
		{REQUESTPACK,  HandleUpdataPacket},      /*�������������Ҫ��������*/    
		{UPGRATEISOK,  HandleUpgrareOK},         /*�������������������������ָ��*/
		{LIGHTLUX,     HandleLUX},               /*������նȷ��ͺ�ظ�*/
		{GATEWAYID,    HandleGWAddr},            /*������նȰ�����������ذ��ַһ�º˶�*/
		{TIMECHECK,    HandleTimeFit},           /*�������������ʱ���׼*/
	};
	
	ret = p;
	
	for (i = 0; i < (strlen(p) - 3); ++i) {
		verify ^= *ret++;
	}
	
	sscanf(p, "%*13s%2s", tmp);
	len = strtol((const char *)tmp, NULL, 16);  /*Э������ݳ���*/
	
	sprintf(buf, "%%*%ds%%2s", 15 + len);
	
	sscanf(p, buf, tmp);
	len = strtol((const char *)tmp, NULL, 16);  /*Э�����У����*/
	
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
			return;
		}
	}	
}


