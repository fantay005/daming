#ifndef __COMMZIGBEE_H__
#define __COMMZIGBEE_H__

#include "stdbool.h"

typedef struct{
	char  MAC_ADDR[5];
	char  NODE_NAME[9];
  char  NODE_TYPE[2];
	char  NET_TYPE[2];
	char  NET_ID[3];
	char  FREQUENCY[2];
	char  ADDR_CODE[2];
	char  TX_TYPE[2];
	char  BAUDRATE[2];
	char  DATA_PARITY[2];
	char  DATA_BIT[2];
	char  SRC_ADR[2];
}ZigBee_Param;

typedef struct{
	unsigned char  STATE[10];
	unsigned char  DIM[5];
	unsigned char  INPUT_VOL[5];
	unsigned char  INPUT_CUR[5];
	unsigned char  INPUT_POW[5];
	unsigned char  LIGHT_VOL[5];
	unsigned char  PFC_VOL[5];
	unsigned char  TEMP[5];
	unsigned char  TIME[8];
}BSN_Data;



extern char HubNode;              //选择中心节点还是配置选项

bool ComxTaskRecieveModifyData(const char *dat, int len);
bool CommxTaskSendData(const char *dat, unsigned char len);

#endif
