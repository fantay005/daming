#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "misc.h" 
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "gsm.h"
#include "norflash.h"
#include "transfer.h"
 
#define MAX_TASK_STACK_SIZE		( configMINIMAL_STACK_SIZE )

#define UART_GET_DATA_TIME      (configTICK_RATE_HZ * 5)

#define Max_com    USART2
#define Max_Irq    USART2_IRQn
#define Max_Baud   9600

#define GPIO_En    GPIOE
#define Max_En     GPIO_Pin_6

#define GPIO_Max   GPIOA
#define Max_Tx     GPIO_Pin_2
#define Max_Rx     GPIO_Pin_3 

static xQueueHandle __maxQueue;

typedef struct {
	unsigned char station;
	unsigned char function;
	unsigned char origin[2];
	unsigned char readbyte[2];
	unsigned char crc[2];
} Info_frame;

static inline void __maxHardwareInit(void) {
	GPIO_InitTypeDef    GPIO_InitStructure;
	USART_InitTypeDef   USART_InitStructure;
	NVIC_InitTypeDef    NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin =  Max_Tx;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_Max, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = Max_Rx;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIO_Max, &GPIO_InitStructure);				

	USART_InitStructure.USART_BaudRate = Max_Baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(Max_com, &USART_InitStructure);
	
	USART_ITConfig(Max_com, USART_IT_RXNE, ENABLE);
	USART_Cmd(Max_com, ENABLE);
	
	USART_ClearFlag(Max_com, USART_FLAG_TXE);
	
	GPIO_InitStructure.GPIO_Pin =  Max_En;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_En, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIO_En, Max_En);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

	NVIC_InitStructure.NVIC_IRQChannel = Max_Irq;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void Max_Tx_dir(void) {
	GPIO_SetBits(GPIO_En, Max_En);
	vTaskDelay(configTICK_RATE_HZ / 100);
}

void Max_Rx_dir(void) {
	vTaskDelay(configTICK_RATE_HZ / 100);
	GPIO_ResetBits(GPIO_En, Max_En);
}


char *Modbusdat (Info_frame *h){
	uint16_t p = 0xffff;
	char i, j;
	uint8_t tmp;
	unsigned char *dat;
	h->station = 0x0A;
	h->function = 0x04;
	h->origin[0] = 0x00;
	h->origin[1] = 0x00;
	h->readbyte[0] = 0x00;
	h->readbyte[1] = 0x02;
	
	dat = (unsigned char *)h;
	
	for(j=0; j<6; j++) {
		tmp = p & 0xff;
		tmp = tmp ^ (*dat++);
		p = (p & 0xff00) | tmp;
		
		for (i=0; i<8; i++) {
			if (p & 0x01) {
				p = p >> 1;
				p = p ^ 0xA001;
			} else {
				p = p >> 1;
			}
		}
	}
	h->crc[0] = p & 0xff;
	h->crc[1] = p >> 8;	
	return (char *)h;
}

void Max_Send_Byte(unsigned char byte){
   USART_SendData(Max_com, byte); 
   while( USART_GetFlagStatus(Max_com,USART_FLAG_TXE)!= SET);          
}

void Max_Send_Str(unsigned char *s, int size){	
   unsigned char i=0; 
	
	 for(; i < size; i++) 
     Max_Send_Byte(s[i]); 
}

static unsigned int LuxValue = 123456;

unsigned short GetLux(void){
	return LuxValue;
}

extern unsigned char *ProtocolMessage(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);

extern char StopLuxDataSend(void);

static void __MaxTask(void *nouse) {
	portBASE_TYPE rc;
	Info_frame frame;
	portTickType curT, UpgradeT = 0xFFFFFFFF, lastT = 0; 
	unsigned int msg; 
	
	Modbusdat(&frame);
	
	while (1) {
		rc = xQueueReceive(__maxQueue, &msg, configTICK_RATE_HZ / 5);
		if (rc == pdTRUE) {		
			unsigned char size, *buf, tmp[10];
	
			sprintf((char *)tmp, "%08d", msg);
			buf = ProtocolMessage("9999999999", "43", (const char *)tmp, &size);
			TransTaskSendData((const char *)buf, size);
			vPortFree(buf);
			
		} else {	    
			  curT = xTaskGetTickCount();
			
				if (((curT - lastT) >= UART_GET_DATA_TIME) && (!StopLuxDataSend())){
					Max_Tx_dir();
					Max_Send_Str((unsigned char *)&frame, 8);
					Max_Rx_dir();
					lastT = curT;
				}
				
				if(UpgradeT == 0xFFFFFFFF){
					UpgradeT = curT;
					continue;
				}
				
				if((curT - UpgradeT) > UART_GET_DATA_TIME * 6){
					GMSParameter g;
					unsigned char size, *buf, tmp[8];			
		
					
					NorFlashRead(NORFLASH_MANAGEM_ADDR, (short *)&g, (sizeof(GMSParameter)  + 1)/ 2);	
					
					sprintf((char *)tmp, "%05X%s", LuxValue, "0");
					
					buf = ProtocolMessage(g.GWAddr, "0F", (const char *)tmp, &size);
					GsmTaskSendTcpData((const char *)buf, size);
					vPortFree(buf);	

					UpgradeT = curT;
				}
		}
	}
}

static char Buffer[9];
static int Index = 0;

void USART2_IRQHandler(void)
{ 
	uint32_t dat;
	if (USART_GetITStatus(Max_com, USART_IT_RXNE) != RESET) {
		dat = USART_ReceiveData(Max_com);
		USART_SendData(UART5, dat);
		USART_ClearITPendingBit(Max_com, USART_IT_RXNE);
		if (Index == 8) {		  
			portBASE_TYPE xHigherPriorityTaskWoken;
			Buffer[Index++] = dat;
	
			LuxValue = (Buffer[3] << 24) + (Buffer[4] << 16) + (Buffer[5] << 8) + Buffer[6];
			if (pdTRUE == xQueueSendFromISR(__maxQueue, &LuxValue, &xHigherPriorityTaskWoken)) {
				if (xHigherPriorityTaskWoken) {
					portYIELD();
				}
			}
			Index = 0;
		} else {
			if(Index == 0) {
				if(dat == 0x0A) {
			    Buffer[Index++] = dat;
				}
			} else if (Index == 1) {
				if(dat == 0x04) {
			    Buffer[Index++] = dat;
				} else {
					Index = 0;
				}
			} else if (Index == 2) {
				if(dat == 0x04) {
			    Buffer[Index++] = dat;
				} else {
					Index = 0;
				}
			} else {
				Buffer[Index++] = dat;
			}
		}
	}
}

static inline void __maxCreateTask(void) {
	__maxQueue = xQueueCreate(5, sizeof( int ));
	xTaskCreate(__MaxTask, (signed portCHAR *) "MAX485", MAX_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5, NULL);
}

void MAX485Init(void) {
	__maxHardwareInit();
	__maxCreateTask();
}
