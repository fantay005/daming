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

#define Download_Store_Addr  (0x08040000)             //升级文件存储地址

extern void STMFLASH_Visit(uint32_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead);

typedef enum{
	ACKERROR = 0,           /*从站应答异常*/
	VERSIONQUERY = 0x0C,    /*网关软件版本号查询*/ 
	LUXVERSION = 0x0E,      /*光照度板软件号查询*/
	ADDRESSQUERY = 0x11,    /*网关地址查询*/
	SETSERVERIP = 0x14,     /*设置网关目标服务器IP*/
	GATEUPGRADE = 0x16,     /*光照传感器网关远程升级*/
	RSSIVALUE = 0x17,       /*GSM信号强度查询*/
	CALLBALANCE = 0x18,     /*手机话卡信息查询*/
	LUXQUERY = 0x20,        /*光照度查询*/
	RETAIN,                 /*保留*/
} GatewayType;


typedef enum{
	REQUESTPACK = 0x40,     /*隧道内网关请求升级包*/
	UPGRATEISOK = 0x41,     /*隧道内网关升级结束*/
	TIMECHECK = 0x42,       /*核对时间*/
	LIGHTLUX = 0x43,        /*环境光照度回复*/
	GATEWAYID = 0x44,       /*网关地址下发*/
	ASKTIME = 0x45,         /*隧道内网关请求时间*/
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

static void HandleGWAddrQuery(ProtocolHead *head, const char *p) {   /**/
	unsigned char *buf, msg[5], size;	
	GMSParameter g;
	
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	
	sscanf(p, "%4s", msg);
	buf = ProtocolRespond(g.GWAddr, head->contr, (const char *)msg, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
}

static bool ResetSoftware = false;   /*软件重启*/

bool IsSoftwareReset(char p){
	if(p == 1)
		return ResetSoftware;
	if(p == 2)
		ResetSoftware = false;
}

static void HandleSetGWServ(ProtocolHead *head, const char *p) {      /*设置网关目标服务器IP*/
	unsigned char *buf, size, tmp[6];
	GMSParameter g;
	
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	sscanf(p, "%[^,]", g.serverIP);
	sscanf(p, "%*[^,]%*c%[^;]", tmp);
	
	g.serverPORT = atoi((const char *)tmp);
	
	NorFlashWrite(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter) + 1) / 2);
	buf = ProtocolRespond(head->addr, head->contr, NULL, &size);
  GsmTaskSendTcpData((const char *)buf, size);
	ProtocolDestroyMessage((const char *)buf);	
	
	ResetSoftware = true;             /*需要软件重启*/
}

extern bool GsmTaskSendAtCommand(const char *atcmd);
extern void SwitchCommand(void);

static void HandleGWUpgrade(ProtocolHead *head, const char *p){             
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
	
	sscanf(p, "%1s", tmp);

	SwitchCommand();
	
	if(strncasecmp(tmp, "A", 1) == 0){
		if (FirmwareUpdateSetMark(mark, host, port, remoteFile, 1)){    //光照传感器DTU，FTP远程升级
			NVIC_SystemReset();
		}
	}
	
	if(strncasecmp(tmp, "B", 1) == 0){
		if (FirmwareUpdateSetMark(mark, host, port, remoteFile, 2)){     //隧道内网关，FTP远程升级
			NVIC_SystemReset();
		}
	}
	vPortFree(mark);
}

static void HandleRSSIQuery(ProtocolHead *head, const char *p) {           //GSM信号查询
	SwitchCommand();	
	GsmTaskSendAtCommand("AT+CSQ\r");
}

static void HandleEGUpgrade(ProtocolHead *head, const char *p) {
	
}

static void HandleLuxVersion(ProtocolHead *head, const char *p){
	unsigned char *buf, size;
	
	buf = ProtocolRespond(head->addr, head->contr, __TARGET_STRING__, &size);
  GsmTaskSendTcpData((const char *)buf, size);
}

extern void __cmd_QUERYFARE_Handler(void);
extern void __cmd_QUERYFLOW_Handler(void);

static void HandleCallBalance(ProtocolHead *head, const char *p) {        /*查询手机卡信息*/
	char tmp[8];
	
	sscanf(p, "%2s", tmp);
	if(strncmp(tmp, "00", 2) == 0){          /*查询手机卡余额*/
		SwitchCommand();
		__cmd_QUERYFARE_Handler();
	} else if(strncmp(tmp, "01", 2) == 0){   /*查询手机卡流量使用*/
		SwitchCommand();
		__cmd_QUERYFLOW_Handler();
	}
}


//static void HandleFlowBalance(ProtocolHead *head, const char *p) {
//	SwitchCommand();
//	__cmd_QUERYFLOW_Handler();
//}

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


/*GW: gateway  网关*/
/*EG: electric quantity gather 电量采集器*/
/*illuminance ： 光照度*/
/*BSN: 钠灯镇流器*/
void GPRSProtocolHandler(ProtocolHead *head, char *p) {
	int i;
	char tmp[3], mold;
	char *ret;
	char verify = 0;
	
	const static ProtocolHandleMap map[] = {        
		{ADDRESSQUERY,   HandleGWAddrQuery},      /*0x11; 网关地址查询*/
		{SETSERVERIP,   HandleSetGWServ},        /*0x14; 设置网关目标服务器IP*/   ///
		{GATEUPGRADE,   HandleGWUpgrade},        /*0x16; 光照传感器网关远程升级*/
		{RSSIVALUE,     HandleRSSIQuery},        /*0x17; GSM模块信号强度查询*/
		{LUXVERSION,    HandleLuxVersion},    /*0x20; 隧道内传感器网关升级*/
		{CALLBALANCE,   HandleCallBalance},      /*0x18; 查询手机卡使用信息*/
//		{FLOWBALANCE,   HandleFlowBalance},      /*0x22; 查询手机使用流量*/
		{LUXQUERY,      HandleLuxQuery},         /*0x20; 查询光照度*/
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

	TransTaskSendData(p, strlen(p));
}

/*下面的程序是用来处理光照度采集板和隧道内网关板之间传输数据的协议*/

static char LuxDataStop = 0;

char StopLuxDataSend(void){
	return LuxDataStop;
}

static void HandleUpdataPacket(ProtocolHead *head, const char *p) {
	unsigned char section, buf[16], temp[1024], i;
	short size;
	
	LuxDataStop = 1;
	
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
	
	LuxDataStop = 0;
	FirmwareUpdaterEraseMark();
}

static void HandleLUX(ProtocolHead *head, const char *p) {
	
}

static void HandleGWAddr(ProtocolHead *head, const char *p) {
	
}

static void HandleTimeFit(ProtocolHead *head, const char *p) {
	
}

static void HandleAskTime(ProtocolHead *head, const char *p) {
	DateTime dateTime;
	uint32_t second;
	unsigned char *buf, tmp[16], size;
	
	second = RtcGetTime();
	SecondToDateTime(&dateTime, second);
	
	sprintf((char *)tmp, "%02d%02d%02d%02d%02d%02d", dateTime.year, dateTime.month, dateTime.date, dateTime.hour, dateTime.minute, dateTime.second);
	buf = ProtocolMessage("9999999999", "42", (const char *)tmp, &size);
	TransTaskSendData((const char *)buf, size);
	vPortFree(buf);	
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
	GMSParameter g;		
	
	const static InternalProtocolHandleMap map[] = {  
		{REQUESTPACK,  HandleUpdataPacket},      /*处理隧道内网关要求升级包*/    
		{UPGRATEISOK,  HandleUpgrareOK},         /*处理隧道内网关升级结束返回指令*/
		{LIGHTLUX,     HandleLUX},               /*处理光照度发送后回复*/
		{GATEWAYID,    HandleGWAddr},            /*处理光照度板与隧道内网关版地址一致核对*/
		{TIMECHECK,    HandleTimeFit},           /*处理隧道内网关时间核准*/
		{ASKTIME,      HandleAskTime},           /*处理隧道内网关请求时间*/
	};
	
	ret = p;
	
	for (i = 0; i < (strlen(p) - 3); ++i) {
		verify ^= *ret++;
	}
	
	sscanf(p, "%*13s%2s", tmp);
	len = strtol((const char *)tmp, NULL, 16);  /*协议的数据长度*/
	
	sprintf(buf, "%%*%ds%%2s", 15 + len);
	
	sscanf(p, buf, tmp);
	len = strtol((const char *)tmp, NULL, 16);  /*协议异或校验码*/
	
	if(verify != len)
		return;	
	
	NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter)  + 1)/ 2);		
	
	if((strncmp((const char *)head->addr, (const char *)g.GWAddr, 10)) && 
		(strncmp((const char *)head->addr, "9999999999", 10) != 0)){   /*若两个板的地址不同，发送核对地址指令*/
		unsigned char *buf, size;
		
		buf = ProtocolMessage(g.GWAddr, "44", NULL, &size);
		TransTaskSendData((const char *)buf, size);
		vPortFree(buf);
		return;
	}
	
	sscanf(p, "%*11s%2s", tmp);
	if(tmp[1] > '9'){
		mold = ((tmp[0] & 0x0f) << 4) | (tmp[1] - 'A' + 10);
	} else {
		if(tmp[0] > '8')
			mold = (((tmp[0]  - '8' - 7)& 0x0f) << 4) | (tmp[1] & 0x0f);
		else
			mold = ((tmp[0] & 0x0f) << 4) | (tmp[1] & 0x0f);
	}
	
	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if ((map[i].type == mold)  || ((map[i].type | 0x80) == mold)){
			map[i].func(head, p + 15);
			return;
		}
	}	
	
	GsmTaskSendTcpData(p, strlen(p));
}


