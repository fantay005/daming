#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "rtc.h"
#include "seven_seg_led.h"
//#include "second_datetime.h"

#define DISPLAY_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 64 )



static void __ledDisplayTask(void *nouse) {
//	DateTime dateTime;
	uint32_t second;
	  
	while (1) {	
		
//	  if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {		
//		  continue;
//	  }

//	  second = RtcGetTime();
//	  SecondToDateTime(&dateTime, second);
	}
}

void DisplayInit(void) {
	xTaskCreate(__ledDisplayTask, (signed portCHAR *) "DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
