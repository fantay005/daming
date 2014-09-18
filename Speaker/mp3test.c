#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "mp3.h"
#include "ff.h"
#include "integer.h"
#include "diskio.h"	
#include "sdcard.h"
#include "soundcontrol.h"

#define mp3_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 512)

static xQueueHandle __VS1003PlayQueue;


void MP3TaskPlay(char *p) {
	char *msg = pvPortMalloc(strlen(p) + 1);
	strcpy(msg, p);
	xQueueSend(__VS1003PlayQueue, &msg, configTICK_RATE_HZ * 5);
}


static void __mp3fillcallbak(void) {
	char *p = (char *)-1;
	xQueueSend(__VS1003PlayQueue, &p, configTICK_RATE_HZ * 5);	
}

static void __mp3TestTask(void *nouse) {
	portBASE_TYPE rc;
	char *msg;
	char *buf = pvPortMalloc(512);
	FIL *fsrc = NULL;
	FRESULT result;
  FATFS fs;
	UINT br;
	char song[16];
	int chapter;
	
	while (1) {
		rc = xQueueReceive(__VS1003PlayQueue, &msg, configTICK_RATE_HZ);		
		if (rc == pdTRUE){
			if (msg == (char *)-1 && fsrc != NULL) {
				result = f_read(fsrc, buf, 100, &br);
				if (result == FR_OK && br > 0) {
					 VS1003_Play_Callbak((const unsigned char*)buf, br, __mp3fillcallbak);
				} else {
					SoundControlSetChannel(SOUND_CONTROL_CHANNEL_MP3, 0); 
					f_close(fsrc);
					vPortFree(fsrc);
					fsrc = NULL;					 	
				}
			} else if (msg != NULL) {
				if (fsrc != NULL) {
					 f_close(fsrc);
				} else {
					 fsrc = pvPortMalloc(sizeof(*fsrc));
				}
				
				if (strncasecmp(msg, "close", 5) == 0) {
					if (fsrc != NULL) {
					  f_close(fsrc);
				  }
					SoundControlSetChannel(SOUND_CONTROL_CHANNEL_MP3, 0); 
					continue;
				}
				
				if(strlen(msg) == 1){
				  chapter = *msg;
					if(chapter < 50){
					  sprintf(song, "%c.mp3", chapter);
					} else {
						sprintf(song, "%d.mp3", (chapter - 50));
					}
				} else {
					chapter = atoi(msg);
					sprintf(song, "%d.mp3", chapter);
				}
				
				result = f_mount(&fs, "0:", 1);
				if (result == FR_OK) {
					printf("File mount success.\n");
				} else {
					printf("File mount Error.\n");
				}
				result = f_open(fsrc, song, FA_OPEN_EXISTING | FA_READ);
				if (result == FR_OK) {
						SoundControlSetChannel(SOUND_CONTROL_CHANNEL_MP3, 1); 
						__mp3fillcallbak();
				} else {
					vPortFree(fsrc);
					fsrc = NULL;
				}
				vPortFree(msg);
			}
		} 	
	}
	f_mount(NULL, "0:", 0);
}

void mp3TestInit(void) {
  __VS1003PlayQueue = xQueueCreate(3, sizeof(char *));
	xTaskCreate(__mp3TestTask, (signed portCHAR *) "mp3", mp3_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
