#ifndef __COMMZIGBEE_H__
#define __COMMZIGBEE_H__

typedef struct{
	unsigned char  MAC_ADDR[4];
	unsigned char  NODE_NAME[8];
	unsigned char  NODE_TYPE[8];
	unsigned char  NET_TYPE[6];
	unsigned char  NET_ID[4];
	unsigned char  FREQUENCY[4];
	unsigned char  DATA_TYPE[5];
	unsigned char  TX_TYPE[6];
	unsigned char  BAUDRATE[6];
	unsigned char  DATA_PARITY[6];
	unsigned char  DATA_BIT[5];
	unsigned char  SRC_ADR[9];
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

#endif
