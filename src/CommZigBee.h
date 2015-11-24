#ifndef __COMMZIGBEE_H__
#define __COMMZIGBEE_H__

#include "stdbool.h"

extern unsigned short Addr;

bool ComxConfigData(const char *dat, int len);
bool CommxTaskSendData(const char *dat, unsigned char len);

#endif
