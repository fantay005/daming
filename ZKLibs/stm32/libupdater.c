#include <ctype.h>
#include <string.h>
#include "stm32f10x_flash.h"
#include "libupdater.h"

#define __firmwareUpdaterInternalFlashMarkSavedAddr 0x0800F80

const unsigned int __firmwareUpdaterActiveFlag = 0xF8F88F8F;
FirmwareUpdaterMark *const __firmwareUpdaterInternalFlashMark = (FirmwareUpdaterMark *)__firmwareUpdaterInternalFlashMarkSavedAddr;

bool FirmwareUpdaterIsValidMark(const FirmwareUpdaterMark *mark) {
	if (mark->SizeOfPAK < 1024){
		return false;
	}

	if (mark->type > 3) {
		return false;
	}

	return true;
}


void FirmwareUpdaterEraseMark(void) {
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_ErasePage(0x0800F800);
	FLASH_Lock();
}



bool FirmwareUpdateSetMark(FirmwareUpdaterMark *tmpMark, int size, UpgradeType type) {
	int i;
	unsigned int *pint;

	tmpMark->RequiredFlag = __firmwareUpdaterActiveFlag;
	tmpMark->SizeOfPAK = size;
	tmpMark->type = type;
	for (i = 0; i < sizeof(tmpMark->timesFlag) / sizeof(tmpMark->timesFlag[0]); ++i) {
		tmpMark->timesFlag[i] = 0xFFFFFFFF;
	}

	if (!FirmwareUpdaterIsValidMark(tmpMark)) {
		return false;
	}

	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_ErasePage(__firmwareUpdaterInternalFlashMarkSavedAddr);

	pint = (unsigned int *)tmpMark;

	for (i = 0; i < sizeof(*tmpMark) / sizeof(unsigned int); ++i) {
		FLASH_ProgramWord(__firmwareUpdaterInternalFlashMarkSavedAddr + i * sizeof(unsigned int), *pint++);
	}

	FLASH_Lock();

	if (__firmwareUpdaterInternalFlashMark->RequiredFlag != tmpMark->RequiredFlag) {
		FirmwareUpdaterEraseMark();
		return false;
	}

	if (__firmwareUpdaterInternalFlashMark->SizeOfPAK != tmpMark->SizeOfPAK) {
		FirmwareUpdaterEraseMark();
		return false;
	}

	if (__firmwareUpdaterInternalFlashMark->type != tmpMark->type) {
		FirmwareUpdaterEraseMark();
		return false;
	}

	for (i = 0; i < sizeof(tmpMark->timesFlag) / sizeof(tmpMark->timesFlag[0]); ++i) {
		if (__firmwareUpdaterInternalFlashMark->timesFlag[i] != tmpMark->timesFlag[i]) {
			FirmwareUpdaterEraseMark();
			return false;
		}
	}

	return true;
}
