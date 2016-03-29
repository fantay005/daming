#ifndef __SHUNCOM_H__
#define __SHUNCOM_H__

#include <stdbool.h>

typedef enum{
	RTOS_ERROR,
	COM_SUCCESS,
	STATUS_FIT,
	COM_FAIL,
	POWER_SHUT,
	HANDLE_ERROR,
	POWER_ENABLE,
}SEND_STATUS;

SEND_STATUS ZigbTaskSendData(const char *dat, int len);

extern char BSNinfor[600][40];           /*保存所有镇流器当前数据*/

#endif
