#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gsm.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "protocol.h"
#include "key.h"

typedef struct {
	unsigned char FH;
	unsigned char AD[4];
	unsigned char CT[2];
	unsigned char LT[2];
} FrameHeader;


typedef struct {
	unsigned char BCC[2];
	unsigned char x03;
} ProtocolTail;

unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size) {
	unsigned char i;
	unsigned int verify = 0;
	unsigned char *p, *ret, tmp[5];
	unsigned char hexTable[] = "0123456789ABCDEF";
	unsigned char len = ((msg == NULL) ? 0 : strlen(msg));
	*size = 9 + len + 3 + 2;
	
	ret = pvPortMalloc(*size + 1);
	
	if(strncmp((const char *)address, "FFFF", 4) == 0){
		*ret = 0xFF;
		*(ret + 1) = 0xFF;
  } else {		
		if(HexSwitchDec){
				sscanf((const char *)address, "%4s", tmp);
				verify = strtol((const char *)tmp, NULL, 16);
				*ret = verify >> 8;
				*(ret + 1) = verify & 0xFF;
		} else {		
				*ret = (address[0] << 4) | (address[1] & 0x0f);
				*(ret + 1) = (address[2] << 4) | (address[3] & 0x0f);
		}		
  }
	{
		FrameHeader *h = (FrameHeader *)(ret + 2);
		h->FH = 0x02;	
		strcpy((char *)&(h->AD[0]), (const char *)address);
		h->CT[0] = control[0];
		h->CT[1] = control[1];
		h->LT[0] = hexTable[(len >> 4) & 0x0F];
		h->LT[1] = hexTable[len & 0x0F];
	}

	if (msg != NULL) {
		strcpy((char *)(ret + 11), msg);
	}
	
	p = ret + 2;
	
	verify = 0;
	for (i = 0; i < (len + 9); ++i) {
		verify ^= *p++;
	}
	
	*p++ = hexTable[(verify >> 4) & 0x0F];
	*p++ = hexTable[verify & 0x0F];
	*p++ = 0x03;
	*p = 0;
	return ret;
}
