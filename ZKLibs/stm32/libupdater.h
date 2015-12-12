#ifndef __LIBUPDATER_H__
#define __LIBUPDATER_H__

#include <stdbool.h>

#define UPDATE_VERSION  0x0100

typedef enum{
	ElectBoard,
	BallastBoard,
	CollectBoard,	
}UpgradeType;

typedef struct {
	unsigned int RequiredFlag;
	unsigned int SizeOfPAK;
	UpgradeType type;
	unsigned int timesFlag[5];
} FirmwareUpdaterMark;

bool FirmwareUpdateSetMark(FirmwareUpdaterMark *tmpMark, const char *host, unsigned short port, const char *remoteFile);

#endif
