#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "rtc.h"
#include "second_datetime.h"
#include "gsm.h"
#include "norflash.h"
#include "transfer.h"

#define SHT_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE)

extern unsigned char *ProtocolMessage(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);

static void __TimeTask(void *nouse) {
	DateTime dateTime;
	uint32_t second;
	 
	while (1) {
		 if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {
			continue;
		 }	 
		 
		 second = RtcGetTime();
		 SecondToDateTime(&dateTime, second);

		if((dateTime.hour == 0x0C) && (dateTime.minute == 0x1E) && (dateTime.second == 0x00)){		/*每天中午重启一次*/
			NVIC_SystemReset();			
		} 
		
		if((dateTime.hour == 0x0C) && (dateTime.minute == 0x2D) && (dateTime.second == 0x00)){	 /*每天和隧道内网关校对一次时间*/
			GMSParameter g;
			unsigned char size, *buf, tmp[8];
			
			NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter)  + 1)/ 2);		
			sprintf((char *)tmp, "%d%d%d", dateTime.hour, dateTime.minute, dateTime.second);
			buf = ProtocolMessage(g.GWAddr, "42", (const char *)tmp, &size);
			TransTaskSendData((const char *)buf, size);
			vPortFree(buf);
			
			vTaskDelay(configTICK_RATE_HZ);
		} 
		
		if((dateTime.date == 0x05) && (dateTime.hour == 0x0A) && (dateTime.minute == 0x00) && (dateTime.second == 0x00)){    /*每月检查一次话费余额和流量使用情况*/
			vTaskDelay(configTICK_RATE_HZ);
		}

	}
}

void TimePlanInit(void) {
	xTaskCreate(__TimeTask, (signed portCHAR *) "TIMING", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}


