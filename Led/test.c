#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sht1x.h"
#include "rtc.h"
#include "seven_seg_led.h"
#include "second_datetime.h"
#include "unicode2gbk.h"
#include "softpwm_led.h"

#define SHT_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 64 )

void BKUI_Prompt(char para) {
  char prompt[36] = {'[', 'm', '5', '3', ']', 's', 'o', 'u', 'n', 'd', '1', '2', '3',//sound123
		                 0xD5,0xFB,  0xB5,0xE3,  0xB1,0xA8,  0xCA,0xB1,  ',',  0xB1,0xBB,  0xBE,0xA9,  0xCA,0xB1,  0xBC,0xE4, 
                     ',',  '8',  0xB5,0xE3,  0xD5,0xFB,}; //整点报时，北京时间八点整   
	if(para >= 0x0A){
		prompt[30] = 0x31;
		prompt[31] = para + 0x27;
	}	else {	
	  prompt[31] = para + 0x31;	
	}		
  SMS_Prompt(prompt, sizeof(prompt));
}

void HALF_Prompt(void) {
	char prompt[21] = {'[', 'm', '5', '3', ']',  's', 'o', 'u', 'n', 'd', '1', '2', '3',//sound123
		                 0xB0,0xEB,  0xB5,0xE3,  0xB1,0xA8,  0xCA,0xB1};  //半点报时

  SMS_Prompt(prompt, sizeof(prompt));
}
static void __ledTestTask(void *nouse) {
	DateTime dateTime;
	uint32_t second;
	int chapter;
	  
	while (1) {
		   if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {
			  continue;
		   }

		   second = RtcGetTime();
	      SecondToDateTime(&dateTime, second);
		   if ((dateTime.hour == 0x00) && (dateTime.minute == 0x00) && (dateTime.second >= 0x00) && (dateTime.second <= 0x05)) {
		   		printf("Reset From Default Configuration\n");
				  vTaskDelay(configTICK_RATE_HZ * 5);
	        NVIC_SystemReset();
		   }
			 
			 if ((dateTime.hour >= 0x08) && (dateTime.hour <= 0x12) && (dateTime.minute == 0x3B) && (dateTime.second == 0x36)){
				  BKUI_Prompt(dateTime.hour);
			 }
			 
			 if ((dateTime.hour >= 0x08) && (dateTime.hour < 0x12) && (dateTime.minute == 0x1D) && (dateTime.second == 0x36)){
				  HALF_Prompt();
			 }
			 
			 if ((dateTime.hour == 0x08) && (dateTime.minute == 0x05) && (dateTime.second == 0x00)){
				 	chapter = dateTime.date;
	        MP3TaskPlay(&chapter);
			 }
			 
			 if ((dateTime.hour == 0x0A) && (dateTime.minute == 0x05) && (dateTime.second == 0x00)){
				 	chapter = dateTime.date + 1;
				 	if(chapter > 31){
						chapter = chapter - 31;
					}
	        MP3TaskPlay(&chapter);
			 }
			 
			 if ((dateTime.hour == 0x0C) && (dateTime.minute == 0x05) && (dateTime.second == 0x00)){
				 	chapter = dateTime.date + 2;
				 	if(chapter > 31){
						chapter = chapter - 31;
					}
	        MP3TaskPlay(&chapter);
			 }
			 
			 if ((dateTime.hour == 0x0F) && (dateTime.minute == 0x05) && (dateTime.second == 0x00)){
				 	chapter = dateTime.date + 3;
				  if(chapter > 31){
						chapter = chapter - 31;
					}
	        MP3TaskPlay(&chapter);
			 }
			 
			 if ((dateTime.hour == 0x11) && (dateTime.minute == 0x05) && (dateTime.second == 0x00)){
				 	chapter = dateTime.date +4;
				  if(chapter > 31){
						chapter = chapter - 31;
					}
	        MP3TaskPlay(&chapter);
			 }

	}
}

void SHT10TestInit(void) {
	xTaskCreate(__ledTestTask, (signed portCHAR *) "TST", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
