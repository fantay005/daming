#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "rtc.h"
#include "seven_seg_led.h"

#define DISPLAY_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE)

static void __ledDisplayTask(void *nouse) {  
	
	while (1) {	
		SevenSegLedSetContent(0x01ab);
	}
}

void DisplayInit(void) {
	xTaskCreate(__ledDisplayTask, (signed portCHAR *) "DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 0, NULL);
}
