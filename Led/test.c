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

#define SHT_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE)

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

	}
}

void TimePlanInit(void) {
	xTaskCreate(__TimeTask, (signed portCHAR *) "TIMING", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}


