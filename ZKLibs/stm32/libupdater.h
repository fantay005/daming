#ifndef __LIBUPDATER_H__
#define __LIBUPDATER_H__

#include <stdbool.h>

#define UPDATE_VERSION  0x0100

typedef struct {
	unsigned int RequiredFlag;
	unsigned int SizeOfPAK;
	unsigned int type;
	unsigned int timesFlag[5];
} FirmwareUpdaterMark;

bool FirmwareUpdateSetMark(FirmwareUpdaterMark *tmpMark, int size, unsigned int type) ;

#endif
