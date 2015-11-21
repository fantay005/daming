#ifndef __COMMZIGBEE_H__
#define __COMMZIGBEE_H__

#include "stdbool.h"

bool ComxConfigData(const char *dat, int len);
bool CommxTaskSendData(const char *dat, unsigned char len);

#endif
