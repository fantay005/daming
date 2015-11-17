#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "fm.h"
#include <stdint.h>
#include <stdbool.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "fm.h"
#include "xfs.h"
#include "soundcontrol.h"

#define AM_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 16)
#define AM_SWITCH_TIME      (configTICK_RATE_HZ * 2)
static xQueueHandle          __queue;
static xSemaphoreHandle      __asemaphore = NULL;



extern uint8_t OperationSi4731_2w(T_OPERA_MODE operation, uint8_t *data, uint8_t numBytes);
extern void ResetSi4731_2w(void);


static T_ERROR_OP Si4731_Power_Down(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_power_down[] = {0x11};

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_power_down[0]), 1);
	if (error_ind) {
		return I2C_ERROR;
	}
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}



static  T_ERROR_OP Si4731_Power_Up(T_POWER_UP_TYPE power_up_type) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_power_up[] = {0x01, 0xC1, 0x05};

	switch (power_up_type) {
	case FM_RECEIVER: {
		Si4731_power_up[1] = 0xD0;
		Si4731_power_up[2] = 0x05;
		break;
	}
	case FM_TRNSMITTER: {
		Si4731_power_up[1] = 0xC2;
		Si4731_power_up[2] = 0x50;
		break;
	}
	case AM_RECEIVER: {
		Si4731_power_up[1] = 0xC1;
		Si4731_power_up[2] = 0x05;
		break;
	}
	}

	ResetSi4731_2w();
	vTaskDelay(configTICK_RATE_HZ / 5);
	error_ind = OperationSi4731_2w(WRITE, &(Si4731_power_up[0]), 3);
	if (error_ind) {
		return I2C_ERROR;
	}
	vTaskDelay(configTICK_RATE_HZ / 2);;
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff)); 
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}

static T_ERROR_OP Si4731_Set_Property_GPO_IEN(void) {
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_set_property[] = {0x12, 0x00, 0x00, 0x01, 0x00, 0xC1};	//set STCIEN, ERRIEN,CTSIEN

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if (error_ind) {
		return I2C_ERROR;
	}
	do {
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
	} while (((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if (loop_counter >= 0xff) {
		return LOOP_EXP_ERROR;
	}
	return OK;
}


static T_ERROR_OP Si4731_Get_INT_status(void)
{
	uint16_t loop_counter = 0;
	uint8_t Si4731_reg_data[32];	
	uint8_t error_ind = 0;
	uint8_t Si4731_Get_INT_status[] = {0x14};	

	error_ind = OperationSi4731_2w(WRITE, &(Si4731_Get_INT_status[0]), 1);
	if(error_ind)
		return I2C_ERROR;
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	
	return OK;
}

/**************************************

Si4731_Set_Property_AM_DEEMPHASIS()

***************************************/

T_ERROR_OP Si4731_Set_Property_AM_DEEMPHASIS(void)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_set_property[] = {0x12,0x00,0x31,0x00,0x00,0x01};	//AM deemphasis is 50us

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

/**************************************

Si4731_Set_Property_AM_Seek_Band_Bottom()

***************************************/

T_ERROR_OP Si4731_Set_Property_AM_Seek_Band_Bottom(void)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_set_property[] = {0x12,0x00,0x34,0x00,0x02,0x0A};	/*0x0208 = 522KHz*/

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

/**************************************

Si4731_Set_Property_AM_Seek_Band_Top()

***************************************/

T_ERROR_OP Si4731_Set_Property_AM_Seek_Band_Top(void)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_set_property[] = {0x12,0x00,0x34,0x01,0x06,0xAE};	/*0x06AE = 1710KHz*/

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}


/**************************************

Si4731_Set_Property_AM_Seek_Space()

***************************************/

T_ERROR_OP Si4731_Set_Property_AM_Seek_Space(void)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_set_property[] = {0x12,0x00,0x34,0x02,0x00,0x09};	//seek space = 0x09  = 9KHz

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}


/**************************************

Si4731_Set_Property_AM_SNR_Threshold()

***************************************/

T_ERROR_OP Si4731_Set_Property_AM_SNR_Threshold(void)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_set_property[] = {0x12,0x00,0x34,0x03,0x00,0x05};	//SNR threshold = 0x0005 = 5dB

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

/**************************************

Si4731_Set_Property_AM_RSSI_Threshold()

***************************************/

T_ERROR_OP Si4731_Set_Property_AM_RSSI_Threshold(void)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_set_property[] = {0x12,0x00,0x34,0x04,0x00,0x0A};	//RSSI threshold = 0x000A = 10dBuV

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_set_property[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

static T_ERROR_OP Si4731_Wait_STC(void) {
	uint16_t loop_counter = 0, loop_counter_1 = 0;
	uint8_t Si4731_reg_data[32];
	uint8_t error_ind = 0;
	uint8_t Si4731_get_int_status[] = {0x14};	//读中断位

	do {
  	vTaskDelay(configTICK_RATE_HZ / 10);
		error_ind = OperationSi4731_2w(WRITE, &(Si4731_get_int_status[0]), 1);

		if (error_ind) {
			return I2C_ERROR;
		}
		do {
			error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
			if (error_ind) {
				return I2C_ERROR;
			}
			loop_counter_1++;
		} while (((Si4731_reg_data[0] & 0x80) == 0) && (loop_counter_1 < 0xff));
		if (loop_counter_1 >= 0xff) {
			return LOOP_EXP_ERROR;
		}
		loop_counter_1 = 0;
		vTaskDelay(configTICK_RATE_HZ / 100);
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if (error_ind) {
			return I2C_ERROR;
		}
		loop_counter++;
		
		if (loop_counter == 0x12C) {
			return LOOP_EXP_ERROR;
		}
		
	} while (((Si4731_reg_data[0] & 0x01) == 0) && (loop_counter < 0x12C));
	
	return OK;
}

/**************************************

static Si4731_AM_Tune_Freq()

***************************************/

static T_ERROR_OP Si4731_AM_Tune_Freq(unsigned short channel_freq)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_tune_freq[] = {0x40,0x00,0x03,0xE8,0x00,0x00};	//0x03E8=1000KHz
	
	Si4731_tune_freq[2] = (channel_freq&0xff00) >> 8;
	Si4731_tune_freq[3] = (channel_freq&0x00ff);	

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_tune_freq[0]), 6);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

/**************************************

static Si4731_AM_Seek_Start()

***************************************/

static T_ERROR_OP Si4731_AM_Seek_Start(T_SEEK_MODE seek_mode)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_seek_start[] = {0x41,0x0C};
			
	
	switch(seek_mode)
	{
		case SEEKDOWN_HALT:
		{
			Si4731_seek_start[1] = 0x00;
			break;
		}
    case SEEKDOWN_WRAP:
    {
    	Si4731_seek_start[1] = 0x04;
    	break;
    }
    case SEEKUP_HALT:
    {
    	Si4731_seek_start[1] = 0x08;
    	break;
    }
    case SEEKUP_WRAP:
    {
    	Si4731_seek_start[1] = 0x0C;
    	break;
    }
  }
	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_seek_start[0]), 2);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	

	return OK;

}

/**************************************

static Si4731_AM_Tune_Status()

***************************************/

static T_ERROR_OP Si4731_AM_Tune_Status(unsigned short *pChannel_Freq, unsigned char *SeekFail, unsigned char *valid_channel)
{
	unsigned short loop_counter = 0;
	unsigned char Si4731_reg_data[32];	
	unsigned char error_ind = 0;
	unsigned char Si4731_fm_tune_status[] = {0x42,0x01};		

	//send CMD
 	error_ind = OperationSi4731_2w(WRITE, &(Si4731_fm_tune_status[0]), 2);
	if(error_ind)
		return I2C_ERROR;

	//wait CTS = 1
	do
	{	
		error_ind = OperationSi4731_2w(READ, &(Si4731_reg_data[0]), 1);
		if(error_ind)
			return I2C_ERROR;	
		loop_counter++;
	}
	while(((Si4731_reg_data[0]) != 0x80) && (loop_counter < 0xff));  //loop_counter limit should guarantee at least 300us
	
	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;	
		
	//read tune status: you should read out: {0x80,0x81,0x03,0xE8,0x2A,0x1A,0x0D,0x95} //Freq=0x03E8=1000KHz, RSSI=0x2A=42dBuV, SNR=0x1A=26dB
	error_ind = OperationSi4731_2w(READ, &Si4731_reg_data[0], 8);	
	if(error_ind)
		return I2C_ERROR;
		
	if(((Si4731_reg_data[1]&0x80) != 0))
		*SeekFail = 1;
	else
		*SeekFail = 0;
		
	if(((Si4731_reg_data[1]&0x01) != 0))
		*valid_channel = 1;
	else
		*valid_channel = 0;
		
	*pChannel_Freq = ((Si4731_reg_data[2] << 8) | Si4731_reg_data[3]);

	return OK;

}

/**************************************

Si4731_Set_AM_Frequency()

***************************************/

T_ERROR_OP Si4731_Set_AM_Frequency(unsigned short channel_freq)
{
	if(Si4731_AM_Tune_Freq(channel_freq) != OK) return ERROR;
	vTaskDelay(configTICK_RATE_HZ / 5);
	if(Si4731_Wait_STC() != OK) return ERROR;	
	return OK;
}

/**************************************

Si4731_AM_Seek()

SeekFail:"no any station" or not when in WRAP mode

***************************************/

T_ERROR_OP Si4731_AM_Seek(T_SEEK_MODE seek_mode, unsigned short *pChannel_Freq, unsigned char *SeekFail)
{
	unsigned char valid_channel;
	unsigned short loop_counter = 0;

	do
	{
		if(Si4731_AM_Seek_Start(seek_mode) != OK) return ERROR;
		vTaskDelay(configTICK_RATE_HZ / 5);
		if(Si4731_Wait_STC() != OK) return ERROR;
		//read seek result:
		if(Si4731_AM_Tune_Status(pChannel_Freq, SeekFail, &valid_channel) != OK) return ERROR;	
		
		loop_counter++;
	}
	while((valid_channel == 0) && (loop_counter < 0xff) && (*SeekFail == 0));  

	if(loop_counter >= 0xff)
		return LOOP_EXP_ERROR;
		
	if((seek_mode == SEEKDOWN_WRAP) || (seek_mode == SEEKUP_WRAP))
		if((valid_channel == 1) && (*SeekFail == 1))
			*SeekFail = 0;

	return OK;
}

/**************************************

Si4731_AM_Seek_All()

***************************************/

T_ERROR_OP Si4731_AM_Seek_All(unsigned short *pChannel_All_Array, unsigned char Max_Length, unsigned char *pReturn_Length)
{
	unsigned char SeekFail;
	unsigned short Channel_Result, Last_Channel = 520;
		
	*pReturn_Length = 0;
	
	if(Si4731_Set_AM_Frequency(522) != OK) return ERROR;
	
	while(*pReturn_Length < Max_Length)
	{
		if(Si4731_AM_Seek(SEEKUP_WRAP, &Channel_Result, &SeekFail) != OK) return ERROR;
			
		if(SeekFail)
			return OK;
		
		if((Channel_Result) <= Last_Channel)	
		{
			if((Channel_Result) == 520)
			{
				*pChannel_All_Array++ = Channel_Result;
				(*pReturn_Length)++;
			}
			return OK;
		}
		else
		{
			*pChannel_All_Array++ = Last_Channel = Channel_Result;
			(*pReturn_Length)++;
		}
	}
	
	return OK;
}

void EXTI3_INTI(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOG, &GPIO_InitStructure);		  


	EXTI_InitStructure.EXTI_Line = EXTI_Line13; 
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; 
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                            
    EXTI_Init(&EXTI_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource13);	  

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;     //选择中断通道1
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占式中断优先级设置为0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        //响应式中断优先级设置为0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                   //使能中断
    NVIC_Init(&NVIC_InitStructure);
}

static char NEXT = 0;
static unsigned short pChannel[30];
static portTickType lastT = 0;
static char choose = 0;
static portTickType upT = 0;
static portTickType downT = 0xFFFFFFFF;

void EXTI15_10_IRQHandler (void)
{
	int curT;
	portBASE_TYPE msg;
	EXTI_ClearITPendingBit(EXTI_Line13);
	if (pdTRUE == xSemaphoreGiveFromISR(__asemaphore, &msg)) {
		curT = xTaskGetTickCount();
		if ((curT - lastT) >= AM_SWITCH_TIME){
		   NEXT ++;
		   if(NEXT > 30){
		      NEXT = 0;
       }
			 
			 if (msg) {
			    taskYIELD();
		   }
		 }
		 
		 if(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13) == 0){
			  downT = xTaskGetTickCount();
		 }

		 if(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13) == 1){
			  upT = xTaskGetTickCount();
			  if(upT > downT){
					if ((upT - downT) >= AM_SWITCH_TIME){
           choose = 1;					
				  }
				}
        downT = 0xFFFFFFFF;					
		 } 		 

		lastT = curT;
	}
}

extern void SI4731Init(void);


void auto_seek_Property(void){
	vTaskDelay(configTICK_RATE_HZ / 10);
	GPIO_SetBits(GPIOB, GPIO_Pin_5);
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Power_Up(AM_RECEIVER);
	vTaskDelay(configTICK_RATE_HZ / 5);
	Si4731_Set_Property_GPO_IEN();
	vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_AM_DEEMPHASIS();
  vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_AM_Seek_Band_Bottom();
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_AM_Seek_Band_Top();
	vTaskDelay(configTICK_RATE_HZ / 10);
	Si4731_Set_Property_AM_Seek_Space();
	vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_AM_SNR_Threshold();
  vTaskDelay(configTICK_RATE_HZ / 10);
  Si4731_Set_Property_AM_RSSI_Threshold();
	vTaskDelay(configTICK_RATE_HZ);
}
	

static  char memory = 0;

static void Broadcast(unsigned short para) {
	int i;
	char buf[8];
	char tune[25] = {0xFD, 0x00, 0x18, 0x01, 0x01, '[', 'm', '5', '1', ']', 's', 'o', 'u', 'n', 'd', '1', '1', '4',//sound114
	                	',', 'a', 'm', ',', '9', '0','8'	};	  //FM90.8!
  sprintf(buf, "%d", para);
  if(para >= 1000){
		tune[21] = buf[0];
		tune[22] = buf[1];
		tune[23] = buf[2];
		tune[24] = buf[3];
	}	else {
		tune[22] = buf[0];
		tune[23] = buf[1];
		tune[24] = buf[2];
	}
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 1);
  for (i = 0; i < 25; i++) {
		USART_SendData(USART3, tune[i]);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
	vTaskDelay(configTICK_RATE_HZ * 5);
	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_XFS, 0);
	recover();
}

void __AMTask(void) {
	unsigned char Return_Length = 0;
	portBASE_TYPE rc;
	__asemaphore = xQueueGenericCreate(1, semSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_BINARY_SEMAPHORE );
	memset(&pChannel[0], 0, 30);
	EXTI3_INTI();
	auto_seek_Property();	
	Si4731_AM_Seek_All(&(pChannel[0]), 30, &Return_Length);
	for (;;) {
	  if (__asemaphore != NULL) {
			 xSemaphoreTake(__asemaphore, portMAX_DELAY);
			 if(choose == 1){
				 choose = 0;
				 if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 0){
			       SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 1);
		     } else {
				     SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 0);
         }
			 }
       if(memory == NEXT) continue;
			 if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 1){
				 if(pChannel[NEXT] != 0){
					  Broadcast(pChannel[NEXT]);
			   }
			 }
	     if(NEXT < Return_Length){
				  printf("%d=%5d\n", NEXT, pChannel[NEXT]);
	        Si4731_Set_AM_Frequency(pChannel[NEXT]);
       } else {
				  NEXT = 0;
				  Broadcast(pChannel[NEXT]);
				  printf("%d=%5d\n", NEXT, pChannel[NEXT]);
				  Si4731_Set_AM_Frequency(pChannel[NEXT]);
       }
			 
			 memory = NEXT;
	  }
  }
}

// void amopen(int freq) {
// 	if ((freq < 875) || (freq > 1080)) {
// 		return;
// 	}
// 	freq = freq * 10;
// 	RST_PIN_INIT;
// 	SDIO_PIN_INIT;
// 	SCLK_PIN_INIT;
// 	RST_LOW;
// 	vTaskDelay(configTICK_RATE_HZ / 10);
// 	RST_HIGH;
// 	vTaskDelay(configTICK_RATE_HZ / 10);
// 	Si4731_Power_Down();
// 	vTaskDelay(configTICK_RATE_HZ / 5);
// 	Si4731_Power_Up(FM_RECEIVER);
// 	vTaskDelay(configTICK_RATE_HZ / 10);
// 	Si4731_Set_Property_GPO_IEN();
// 	vTaskDelay(configTICK_RATE_HZ / 10);
//   Si4731_Set_Property_FM_DEEMPHASIS();
//   vTaskDelay(configTICK_RATE_HZ / 10);
//   Si4731_Set_Property_FM_SNR_Threshold();
//   vTaskDelay(configTICK_RATE_HZ / 10);
//   Si4731_Set_Property_FM_RSSI_Threshold();
// 	SoundControlSetChannel(SOUND_CONTROL_CHANNEL_FM, 1);
// 	vTaskDelay(configTICK_RATE_HZ / 10);
// 	Si4731_Set_FM_Frequency(freq);
// }

void AMInit(void) {
	SI4731Init();
	xTaskCreate(__AMTask, (signed portCHAR *) "AM", AM_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 10, NULL);
}