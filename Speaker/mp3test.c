#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "mp3.h"

#define mp3_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 64 )

static xQueueHandle __VS1003PlayQueue;

static void __mp3TestTask(void *nouse) {
	portBASE_TYPE rc;
	char *msg;
	
	__VS1003PlayQueue = xQueueCreate(5, sizeof(char *));
	
	while (1) {
		rc = xQueueReceive(__VS1003PlayQueue, &msg, portMAX_DELAY);		
		if (rc == pdTRUE){
			msg = pvPortMalloc(strlen(msg) +1);
		  VS1003_Play((const unsigned char*)&msg, strlen(msg));
			vPortFree(msg);
			vTaskDelay(configTICK_RATE_HZ / 500);
		}
	}
}

void mp3TestInit(void) {

	xTaskCreate(__mp3TestTask, (signed portCHAR *) "mp3", mp3_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
