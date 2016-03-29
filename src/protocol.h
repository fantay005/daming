#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "gsm.h"

typedef struct {
	unsigned char header;
	unsigned char addr[10];
	unsigned char contr[2];
	unsigned char lenth[2];
} ProtocolHead;

typedef struct {
	unsigned char FH;
	unsigned char AD[4];
	unsigned char CT[2];
	unsigned char LT[2];
} FrameHeader;

typedef struct{
	unsigned char GatewayID[6];          /*������ݱ�ʶ*/
	unsigned char Longitude[10];         /*����*/
	unsigned char Latitude[10];          /*γ��*/
	unsigned char FrequPoint;            /*ZIGBEEƵ��*/
	unsigned char IntervalTime[2];       /*�Զ��ϴ�����ʱ����*/
	unsigned char Ratio[2];              /*����������*/
	unsigned char EmbedInformation;      /*��Ϣ�����ʶ*/
}GatewayParam1;                        /*���ز�������֡1*/


typedef struct{
	unsigned char HVolLimitValL1[4];      /*�ܻ�·L1/L2/L3�ߵ�ѹ�޶�ֵ*/
	unsigned char HVolLimitValL2[4]; 
	unsigned char HVolLimitValL3[4]; 
	unsigned char LVolLimitValL1[4];      /*�ܻ�·L1/L2/L3�͵�ѹ�޶�ֵ*/ 
	unsigned char LVolLimitValL2[4]; 
	unsigned char LVolLimitValL3[4]; 
	unsigned char NoloadCurLimitValL1[4]; /*�ܻ�·L1/L2/L3/N����ص����޶�ֵ*/
	unsigned char NoloadCurLimitValL2[4];
	unsigned char NoloadCurLimitValL3[4];
	unsigned char NoloadCurLimitValN[4];
	unsigned char PhaseCurLimitValL1[4];  /*�ܻ�·A/B/C/N������޶�ֵ*/
	unsigned char PhaseCurLimitValL2[4];
	unsigned char PhaseCurLimitValL3[4];
	unsigned char PhaseCurLimitValN[4];
	unsigned char NumbOfCNBL;            /*��������������*/
	unsigned char OtherWarn[2];          /*��������*/ 
	unsigned char SetWarnFlag;
}GatewayParam3;

typedef struct{
	unsigned char AddrOfZigbee[4];   /*ZIGBEE��ַ*/
	unsigned char NorminalPower[4];  /*��ƹ���*/ 
	unsigned char Loop;              /*������·*/ 
	unsigned char LightPole[4];      /*�����Ƹ˺�*/ 
	unsigned char LightSourceType;   /*��Դ����*/ 
	unsigned char LoadPhaseLine;     /*��������*/ 
	unsigned char Attribute[2];      /*��/��/Ͷ����*/ 
	unsigned char TimeOfSYNC[12];    /*�Ʋ���ͬ��ʱ��*/
	unsigned char Empty;
	unsigned char Erasetimes;        /*�Ʋ�ȫ����������*/
	unsigned char CommState;         /*ͨ��״̬*/
	unsigned short InputPower;       /*���빦��*/
	unsigned int UpdataTime;         /*�ϴ�����ʱ��*/
}Lightparam;


typedef struct{
	unsigned char AddrOfZigb[4];    /*Zigbee��ַ*/
//	unsigned char Cache;						/*һ����������*/
	unsigned char SchemeType[2];   /*��������*/
	unsigned char DimmingNOS;      /*�������*/
	unsigned char FirstDCTime[4];  /*��һ�ε������ʱ��*/
	unsigned char FirstDPVal[2];   /*��һ�ε��⹦��ֵ*/
	unsigned char SecondDCTime[4]; /*�ڶ��ε������ʱ��*/
	unsigned char SecondDPVal[2];  /*�ڶ��ε��⹦��ֵ*/
	unsigned char ThirdDCTime[4];  /*�����ε������ʱ��*/
	unsigned char ThirdDPVal[2];   /*�����ε��⹦��ֵ*/
	unsigned char FourthDCTime[4]; /*���Ķε������ʱ��*/
	unsigned char FourthDPVal[2];  /*���Ķε��⹦��ֵ*/
	unsigned char FifthDCTime[4];  /*����ε������ʱ��*/
	unsigned char FifthDPVal[2];   /*����ε��⹦��ֵ*/
	unsigned char SYNCTINE[12];    /*����ͬ����ʶ*/
}StrategyParam;


void ProtocolHandler(ProtocolHead *head, char *p);
unsigned char *ProtocolRespond(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);
unsigned char *ProtocolToElec(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);
unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size);

#endif
