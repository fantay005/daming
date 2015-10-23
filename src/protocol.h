#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

typedef enum{
	DIMMING = 0x04,         /*调光*/
	LAMPSWITCH = 0x05,      /*开关灯*/
	READDATA = 0x06,        /*读镇流器数据*/
	RETAIN,                 /*保留*/
} GatewayType;

#endif
