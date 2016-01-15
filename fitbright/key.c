#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "key.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include "zklib.h"
#include "ili9320.h"
#include "ConfigZigbee.h"
#include "sdcard.h"
#include "math.h"
#include "CommZigBee.h"
#include "norflash.h"

#define KEY_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 512)

static xQueueHandle __KeyQueue;

#define KEY_STATE_0    0       //��ʼ״̬
#define KEY_STATE_1    1       //����ȷ��״̬
#define KEY_STATE_2    2       //�ȴ������ͷ�״̬

#define GPIO_KEY_0     GPIOA
#define Pin_Key_0      GPIO_Pin_4

#define GPIO_KEY_1     GPIOA
#define Pin_Key_1      GPIO_Pin_1

#define GPIO_KEY_2     GPIOA
#define Pin_Key_2      GPIO_Pin_0

#define GPIO_KEY_3     GPIOC
#define Pin_Key_3      GPIO_Pin_3

#define GPIO_KEY_4     GPIOC
#define Pin_Key_4      GPIO_Pin_2

#define GPIO_KEY_5     GPIOC
#define Pin_Key_5      GPIO_Pin_1

#define GPIO_KEY_6     GPIOA
#define Pin_Key_6      GPIO_Pin_5

#define GPIO_KEY_7     GPIOA
#define Pin_Key_7      GPIO_Pin_6

#define GPIO_KEY_8     GPIOA
#define Pin_Key_8      GPIO_Pin_7

#define GPIO_KEY_9     GPIOC
#define Pin_Key_9      GPIO_Pin_4

#define GPIO_KEY_UP    GPIOC
#define Pin_Key_UP     GPIO_Pin_5            //���ϼ�

#define GPIO_KEY_DOWN  GPIOB
#define Pin_Key_DOWN   GPIO_Pin_0            //���¼�

#define GPIO_KEY_LF    GPIOB
#define Pin_Key_LF     GPIO_Pin_1            //�����

#define GPIO_KEY_RT    GPIOB
#define Pin_Key_RT     GPIO_Pin_2            //���Ҽ�

#define GPIO_KEY_MODE  GPIOB
#define Pin_Key_MODE   GPIO_Pin_13           //ģʽ��

#define GPIO_KEY_MENU  GPIOB
#define Pin_Key_MENU   GPIO_Pin_14           //���ܼ�

#define GPIO_KEY_INPT  GPIOB
#define Pin_Key_INPT   GPIO_Pin_15           //�����л��� 

#define GPIO_KEY_DEL   GPIOF                         
#define Pin_Key_DEL    GPIO_Pin_7            //ɾ����

#define GPIO_KEY_OK     GPIOF
#define Pin_Key_OK      GPIO_Pin_6           //ȷ����

/*����Ķ�������ԶŹ������嶨��*/

#define KEY_1        GPIO_ReadInputDataBit(GPIO_KEY_6,Pin_Key_6) 
#define KEY_4        GPIO_ReadInputDataBit(GPIO_KEY_7,Pin_Key_7)
#define KEY_5        GPIO_ReadInputDataBit(GPIO_KEY_8,Pin_Key_8)
#define KEY_6        GPIO_ReadInputDataBit(GPIO_KEY_9,Pin_Key_9)
#define KEY_7        GPIO_ReadInputDataBit(GPIO_KEY_UP,Pin_Key_UP)
#define KEY_8        GPIO_ReadInputDataBit(GPIO_KEY_DOWN,Pin_Key_DOWN)
#define KEY_9        GPIO_ReadInputDataBit(GPIO_KEY_0,Pin_Key_0)
#define KEY_INPUT    GPIO_ReadInputDataBit(GPIO_KEY_1,Pin_Key_1)
#define KEY_0        GPIO_ReadInputDataBit(GPIO_KEY_2,Pin_Key_2)
#define KEY_CONF     GPIO_ReadInputDataBit(GPIO_KEY_3,Pin_Key_3)
#define KEY_DN       GPIO_ReadInputDataBit(GPIO_KEY_4,Pin_Key_4)
#define KEY_2        GPIO_ReadInputDataBit(GPIO_KEY_5,Pin_Key_5)
#define KEY_LF       GPIO_ReadInputDataBit(GPIO_KEY_DEL,Pin_Key_DEL)
#define KEY_RT       GPIO_ReadInputDataBit(GPIO_KEY_OK,Pin_Key_OK)
#define KEY_MENU     GPIO_ReadInputDataBit(GPIO_KEY_MENU,Pin_Key_MENU)
#define KEY_OK       GPIO_ReadInputDataBit(GPIO_KEY_LF,Pin_Key_LF)
#define KEY_3        GPIO_ReadInputDataBit(GPIO_KEY_INPT,Pin_Key_INPT)
#define KEY_UP       GPIO_ReadInputDataBit(GPIO_KEY_MODE,Pin_Key_MODE)
#define KEY_RET      GPIO_ReadInputDataBit(GPIO_KEY_RT,Pin_Key_RT)

/*���¶԰������ж���*/


typedef enum{
	KEY_SEND_DATA,
	KEY_NULL,
}KeyTaskMsgType;

typedef struct {
	/// Message type.
	KeyTaskMsgType type;
	/// Message lenght.
	unsigned char length;
} KeyTaskMsg;

static KeyTaskMsg *__KeyCreateMessage(KeyTaskMsgType type, const char *dat, unsigned char len) {
  KeyTaskMsg *message = pvPortMalloc(ALIGNED_SIZEOF(KeyTaskMsg) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

static inline void *__KeyGetMsgData(KeyTaskMsg *message) {
	return &message[1];
}

void key_gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 

	GPIO_InitStructure.GPIO_Pin = Pin_Key_0;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_0, &GPIO_InitStructure);
	
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//PB3������ͨIO
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_1;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_1, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_2;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_2, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_3;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_3, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_4;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_4, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_5;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_5, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_6;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_6, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_7;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_7, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_8;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_8, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_9;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_9, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_UP;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_UP, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_DOWN;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_DOWN, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_MODE;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_MODE, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_MENU;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_MENU, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_DEL;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_DEL, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_OK;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_OK, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_LF;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_LF, &GPIO_InitStructure);
		
	GPIO_InitStructure.GPIO_Pin = Pin_Key_RT;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_RT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Pin_Key_INPT;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_INPT, &GPIO_InitStructure);
}

void TIM3_Init(void){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
 
	TIM_TimeBaseStructure.TIM_Period = 500; 
	TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1; 
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); 
 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);  
 
	TIM_Cmd(TIM3, ENABLE);  
}

static char IME = 0;                    //��ĸ���������뷨�л�����Ϊ1ʱ��1~6���ΪA~F

KeyPress keycode(void){                 //����б仯�������豻ȷ��
	if(KEY_0 == 0){
		return KEY0;
	} else if(KEY_1 == 0){
		return KEY1;
	} else if(KEY_2 == 0){
		return KEY2;
	} else if(KEY_3 == 0){
		return KEY3;
	} else if(KEY_4 == 0){
		return KEY4;
	} else if(KEY_5 == 0){
		return KEY5;
	} else if(KEY_6 == 0){
		return KEY6;
	} else if(KEY_7 == 0){
		return KEY7;
	} else if(KEY_8 == 0){
		return KEY8;
	} else if(KEY_9 == 0){
		return KEY9;
	} else if(KEY_UP == 0){
		return KEYUP;
	} else if(KEY_DN == 0){
		return KEYDN;
	} else if(KEY_LF == 0){
		return KEYLF;
	} else if(KEY_RT == 0){
		return KEYRT;
	} else if(KEY_CONF == 0){
		return KEYCONF;
	} else if(KEY_MENU == 0){
		return KEYMENU;
	} else if(KEY_INPUT == 0){
		return KEYINPT;
	} else if(KEY_OK == 0){
		return KEYOK;
	} else if(KEY_RET == 0){
		return KEYRETURN;
	} else {
		return NOKEY;
	}
}

static KeyPress KeyConfirm = NOKEY;         //�ϵ�󰴼��ĳ�ʼ״̬
Dis_Type  InterFace = Open_GUI;             //�ϵ����ʾ����ĳ�ʼ״̬
static Dis_Type  LastFace = Open_GUI;       //��һ����ʾ����

char StatusOfInterface(void){
	return InterFace;
}

void key_driver(KeyPress code)              //�����Ч����
{
	static uint32_t key_state = KEY_STATE_0;
	static KeyPress	code_last = NOKEY;
	

	switch (key_state)
	{
		case KEY_STATE_0:                        //��ʼ״̬
			if (code != NOKEY){
				code_last = code;
				key_state = KEY_STATE_1;             //����м������£���¼��ֵ������������������ȷ��״̬
			}
			break;
		case KEY_STATE_1:                        //����ȷ��״̬
			if (code != code_last)
				key_state = KEY_STATE_2;             //�ȴ������ͷ�״̬
			break;
		case KEY_STATE_2:
			if (code != code_last) {               //�ȵ������ͷ� �����س�ʼ״̬
				KeyConfirm = code_last;              //���ؼ�ֵ
				key_state = KEY_STATE_0;
			}
			break;
	}
}

static void InputChange(void){                //��ʾ��ǰ���뷨
	char tmp[5];
	if(IME == 0)
		strcpy(tmp, "123");
	else
		strcpy(tmp, "ABC");
	
	if((!HexSwitchDec) && ((InterFace == Address_Set) || (InterFace == Address_Choose)))
		strcpy(tmp, "   ");
	Ili9320TaskInputDis(tmp, strlen(tmp) + 1);
}

static char times = 1;             			//��ʱ���жϼ���
static char count = 0;            	 	  //�������ּ��ĸ���
static char dat[9];               	 	  //�������ּ����
static unsigned char wave = 1;    	 	  //����
static unsigned char page = 1;    	 	  //ҳ��
static unsigned char MaxPage = 1; 	 	  //���ҳ��
static unsigned char MaxLine = 1;     	//һҳ���������������ѡ��
static unsigned char OptDecide = 0; 		//ȷ��ѡ��
pro Project = Pro_Null;          //��ʼ����ĿΪ��
unsigned char GateWayID = 0;     //�������к�
unsigned char FrequencyDot = 0;  //��ʼƵ��Ϊ��
unsigned int  ZigBAddr = 1;     	//��ʼZigBee��ֵַ
char Config_Enable = 0;          //���ü�ʹ������ģ�鹦�ܣ�1Ϊ��ʼ���ã�2Ϊ������������
static char ParamOrAddr = 1;     //��ַ�͵������л���־��1Ϊ��ַ��2Ϊ������

char Digits = 1;             //�������޸ĵ�ַ��λ��
char MaxBit = 4;             //���λ��
char BaseBit = 40;           //��ʼ���ֵ� ��ʼλ

static unsigned int Loop = 0;        //��·
static unsigned int MainRoad = 0;    //����
static unsigned int AssistRoad = 0;  //����
static unsigned char Flood = 0;      //Ͷ��

static char lastData = 0;    //��һ���޸ĵ�ZigBee��ַ��ֵ

char NodeAble = 0;            //���Ľڵ��Ƿ��Ѿ�����
static char NodeFrequ = 0;   //���Ľڵ��Ƶ��
static char NodeID = 0;      //���Ľڵ������ID

extern unsigned char NumOfPage(void);

unsigned char ProMaxPage(void){         //ȷ����ʾ���͵����ҳ��
	MaxPage = NumOfPage() / 15 + 1;
	return page;
}

static char U3IRQ_Enable = 0;             //ʹ�ܴ���3��������

char Com3IsOK(void){
	if(InterFace == Address_Set)
		U3IRQ_Enable = 1;
	else if(InterFace == Config_Set)
		U3IRQ_Enable = 2;
	else if(InterFace == Config_DIS)
		U3IRQ_Enable = 3;
	else
		U3IRQ_Enable = 0;
	return U3IRQ_Enable;
}   

extern unsigned char *GWname(void);      //��������

bool DisStatus(char type, char param){              //�ж�Dis_Type��������������ֵ�ڰ���ȷ�ϼ�����Ҫ����������
	char tmp[40];
	KeyTaskMsg *msg;
	
	sprintf((char *)tmp, "%s", GWname());
	switch (type){
		case 2:
			return true;
		case 3:
			return true;
		case 4:                         //����ģʽ��
			
			switch(param){
				case 1:
					if(Project == Pro_Null)
						return false;
					return true;
				case 2:
					if(tmp[0] == 0)           //û��ѡ������״����
						return false;
					if(NumOfFrequ < 2)        //ֻ��һ��Ƶ��״����
						return false;
					return true;
				case 3:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 4:
					msg = __KeyCreateMessage(KEY_SEND_DATA, "SHUNCOM\r\n", 10);
					xQueueSend(__KeyQueue, &msg, 10);				
					return true;
				case 5:
					return true;
				default:
					return false;
				
			}
		case 5:                          //ά��ģʽ��                        
	
			switch(param){
				case 1:
					if(Project == Pro_Null)
						return false;
					return true;
				case 2:
					if(tmp[0] == 0)           //û��ѡ������״����
						return false;
					if(NumOfFrequ < 2)        //ֻ��һ��Ƶ��״����
						return false;
					return true;
				case 3:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 4:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 5:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					if(NodeAble == 0)
						return false;
					return true;
				case 7:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					if(HubNode == 0)
						return false;            //û���������Ľڵ�
					return true;
				case 8:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					if(HubNode == 0)
						return false;            //û���������Ľڵ�
					return true;
				case 9:
					if(ZigBAddr == 0)         //û��ѡ��ZigBee��ַ״����
						return false;
					if(HubNode == 0)
						return false;            //û���������Ľڵ�
					return true;
				default:
					return false;
				
			}
		case 6:                         //����ģʽ��
			USART_Cmd(USART3, DISABLE);
			USART_Cmd(USART2,ENABLE);
		
			switch(param){
				case 1:
					if(Project == Pro_Null)
						return false;
					return true;
				case 2:
					if(tmp[0] == 0)           //û��ѡ������״����
						return false;
					if(NumOfFrequ < 2)   //ֻ��һ��Ƶ��״����
						return false;
					return true;
				case 3:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 4:
					if(NodeAble == 0)
						return false;
					return true;
				case 5:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 6:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 7:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 8:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				case 9:
					if(FrequencyDot == 0)     //û��ѡ��Ƶ��״����
						return false;
					return true;
				default:
					return false;
				
			}
		case 7:
			return true;
		case 8:
			if(Project == Pro_Null)     //û��ѡ����Ŀ״����
				return false;
			return true;
		case 9:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 11:
			return true;
		case 12:                     //û��ѡ����Ŀ״����
			if(Project == Pro_Null)
				return false;
			
			return true;
		case 13:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 14:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 15:
			if(Project == Pro_Null)    //û��ѡ����Ŀ״����
				return false;
			return true;
		case 16:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 17:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 18:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 19:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 20:
			if(tmp[0] == 0)            //û��ѡ������״����
				return false;
			return true;
		case 21:
			if(tmp[0] == 0)
				return false;
			return true;
		case 22:
			if(tmp[0] == 0)
				return false;
			return true;
		case 23:
			return true;
		case 25:
			switch(param){
				case 1:
					return true;
				case 2:
					return true;
				default:
					break;			
			}
		  default:
			  return false;
	}	
}

char JudgeMaxNum(void){
	if(MaxPage > page)
		MaxLine = 15;
	else
		MaxLine = NumOfPage()%15;
	
	if(InterFace == Light_Attrib)
		MaxLine = 4;
	return MaxLine;
}



void ChooseLine(void){                         //ȷ����������ҳ��
	unsigned char tmp[2], i;
	
	JudgeMaxNum();
	
	if(KeyConfirm == KEYUP){
		wave--;
		if(wave < 1)
			wave = MaxLine;
	} else if(KeyConfirm == KEYDN){
		wave++;
		if(wave > MaxLine)
			wave = 1;
	} else if(KeyConfirm == KEYLF){	
		if((InterFace != GateWay_Set) && (InterFace != GateWay_Choose) && (InterFace != GateWay_Decide))
			return;
		if(page > 1){
			page--;
			wave = 1;
		} else {
			page = MaxPage;
			if(MaxPage != 1)
				wave = 1;
		}
		
		tmp[0] = InterFace;
		tmp[1] = KeyConfirm;
		SDTaskHandleKey((const char *)tmp, 2);
		
		return;
	} else if(KeyConfirm == KEYRT){
		if((InterFace != GateWay_Set) && (InterFace != GateWay_Choose) && (InterFace != GateWay_Decide))
			return;
		
		if(page < MaxPage){
			wave = 1;
			page++;
		} else {
			page = 1;
			if(MaxPage != 1)
				wave = 1;
		}
		
		tmp[0] = InterFace;
		tmp[1] = KeyConfirm;
		SDTaskHandleKey((const char *)tmp, 2);
		
		return;
	} else if(KeyConfirm == KEYOK){
		
		if(DisStatus(InterFace, wave)){
					
			tmp[0] = InterFace;
			tmp[1] = wave;
			SDTaskHandleKey((const char *)tmp, 2);             //���Ͱ�����������
			OptDecide = 1;
		}
						
		return;
	} 
	
	tmp[0] = wave;
	tmp[1] = 0;
	i = InterFace;
	if((i != Intro_GUI) && (i != Config_Set) && (i != Config_DIS) && (i != Read_Data) && (i != Light_Attrib))
		Ili9320TaskLightLine((const char *)tmp, strlen((const char *)tmp));
}

static char DimmValue = 0;               //����ֵ
	
void __handleAdvanceSet(void){                          //�߼����������£�����������
	portBASE_TYPE xHigherPriorityTaskWoken;
	char tmp[12] = {0};
	
	KeyTaskMsg *msg;
	char hex2char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'L'};

	
	if((KeyConfirm >= KEY0) && (KeyConfirm <= KEYL)){		
		dat[count++] = hex2char[KeyConfirm - 1];
		dat[count] = 0;
		if(count > 6){
			count = 0;
			dat[0] = 0;
		}
	
		Ili9320TaskOrderDis(dat, strlen(dat) + 1);
		KeyConfirm = NOKEY;
		return;
	} else if(KeyConfirm == KEYOK){
		count = 0;
  } else if(KeyConfirm == KEYRT){
		strcpy(dat, "SHUNCOM ");
  } else if(KeyConfirm == KEYINPT){
		InputChange();
		KeyConfirm = NOKEY;
		return;
  } else if(KeyConfirm == KEYUP){
		strcpy(tmp, "UP");
		Ili9320TaskUpAndDown(tmp, strlen(tmp) + 1);
		KeyConfirm = NOKEY;
		return;
  } else if(KeyConfirm == KEYDN){
		strcpy(tmp, "DOWN");
		Ili9320TaskUpAndDown(tmp, strlen(tmp) + 1);
		KeyConfirm = NOKEY;
		return;
  } else if(KeyConfirm == KEYLF){
		if(count > 0){
			count--;
			dat[count] = 0;
		}
		Ili9320TaskOrderDis(dat, strlen(dat) + 1);
		KeyConfirm = NOKEY;
		return;
  } else {
		KeyConfirm = NOKEY;
		return;
	}
	
	msg = __KeyCreateMessage(KEY_SEND_DATA, dat, strlen(dat) + 1);
	KeyConfirm = NOKEY;
	
	Ili9320TaskOrderDis(dat, strlen(dat) + 1);
	if (pdTRUE == xQueueSendFromISR(__KeyQueue, &msg, &xHigherPriorityTaskWoken)) {
		if (xHigherPriorityTaskWoken) {
			portYIELD();
		}
	}
  memset(dat, 0, 9);
} 

void DisplayNodeInfo(void){

	
}


void DisplayProjet(void){
	char buf[20] = {0};

	if(Project == Pro_Null)
		return;
	
	if(Project == Pro_BinHu){
		sprintf(buf, "%s", "�Ϸ�/����/");
	} else if(Project == Pro_ChanYeYuan){
		sprintf(buf, "%s", "�Ϸ�/��ɽ��ҵ԰/");
	} else if(Project == Pro_DaMing){
		sprintf(buf, "%s", "�Ϸ�/��������/");
	} else if(Project == Pro_YaoHai){
		sprintf(buf, "%s", "�Ϸ�/����/");
	} else if(Project == Pro_JingKai){
		sprintf(buf, "%s", "�Ϸ�/����/");
	}
	
	Ili9320TaskDisGateWay(buf, strlen(buf) + 1);
}

extern char *LPAttribute(void);

void DisplayInformation(void){                       //��ʾѡ�����Ŀ�����أ�Ƶ���
	char buf[80], tmp[40], dat[10] = {0}, para[20] = {0};
	
	if(Project == Pro_Null)
		return;

  if((FrequencyDot == 1) && (NumOfFrequ == 1)){      //�ж�����Ƶ��
		sprintf(tmp, "/ΨһƵ��:%02X/����ID:%02X/", FrequPoint1, NetID1);
	} else if(FrequencyDot == 1){
		sprintf(tmp, "/��һƵ��:%02X/����ID:%02X/", FrequPoint1, NetID1);
	} else if(FrequencyDot == 2){
		sprintf(tmp, "/�ڶ�Ƶ��:%02X/����ID:%02X/", FrequPoint2, NetID2);
	} else if((FrequencyDot == 0) && (NumOfFrequ != 1)){
		tmp[0] = 0;
	} else {
		tmp[0] = 0;
	}
	
	if(tmp[0] != 0)
		if(HexSwitchDec)
			sprintf(dat, "��ַ:%04X/", ZigBAddr);
		else
			sprintf(dat, "��ַ:%04d/", ZigBAddr);
	
	if(Project == Pro_BinHu){
		sprintf(para, "%s", "�Ϸ�/����/");
	} else if(Project == Pro_ChanYeYuan){
		sprintf(para, "%s", "�Ϸ�/��ɽ��ҵ԰/");
	} else if(Project == Pro_DaMing){
		sprintf(para, "%s", "�Ϸ�/��������/");
	} else if(Project == Pro_YaoHai){
		sprintf(para, "%s", "�Ϸ�/����/");
	} else if(Project == Pro_JingKai){
		sprintf(para, "%s", "�Ϸ�/����/");
	}
	
	sprintf(buf, "%s%s%s%s%s", para, GWname(), tmp, dat, LPAttribute());
	Ili9320TaskDisGateWay(buf, strlen(buf) + 1);
}

void LightParamDisplay(void){
	char buf[40];
	
	if(Loop)
		sprintf(buf, " ��·ѡ��:  %-8X", Loop);
	else
		sprintf(buf, " ��·ѡ��:");	
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	

	if(MainRoad)
		sprintf(buf, " ����ѡ��:  %-8X", MainRoad);		
	else
		sprintf(buf, " ����ѡ��:");
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	

	if(AssistRoad)
		sprintf(buf, " ����ѡ��:  %-8X", AssistRoad);	
	else
		sprintf(buf, " ����ѡ��:");
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	
	if(Flood)
		sprintf(buf, " Ͷ��ѡ��:  ����   ����");
	else
		sprintf(buf, " Ͷ��ѡ��:  ����   ����");
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);

}

void DisplayNode(void){
	char buf[40];
	
	if(NodeAble){
		sprintf(buf, "�����õ����Ľڵ�Ƶ��:%02X  ����ID:%02X", NodeFrequ, NodeID);
	} else{
		sprintf(buf, "��ѡ���������Ľڵ������... ...");
	}
	Ili9320TaskDisNode(buf, strlen(buf) + 1);
}


void __AddrConfig(void){                     //����ZigBee��ַ������ʾ
	char buf[5];
	
	if(HexSwitchDec)
		sprintf(buf, "\r\n%04X", ZigBAddr);
	else
		sprintf(buf, "\r\n%04d", ZigBAddr);
	Ili9320TaskClear("C", 2);
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	
	buf[0] = Digits + BaseBit;
	buf[1] = 0;
	Ili9320TaskLightByte(buf, strlen(buf) + 1);
}

extern unsigned char *DataSendToBSN(unsigned char control[2], unsigned char address[4], const char *msg, unsigned char *size);            

void ParamDimm(unsigned char ctl[2]){                      //�����Բ���
	char HexToChar[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	char ret[32], *p, tmp[5], *buf;
	char i, j, len;
	
	buf = ret;
	if(ParamOrAddr == 2){
		if(Flood)
			*buf++ = '8';
		else
			*buf++ = '9';
		
		for(i = 0, j = 0; i < 8; i++){
			if((Loop >> (i * 4)) & 0x0F)
				j++;
		}
		*buf++ = HexToChar[j];
		
		for(i = 0, j = 0; i < 8; i++){
			if((MainRoad >> (i * 4)) & 0x0F)
				j++;
		}
		*buf++ = HexToChar[j];
		
		for(i = 0, j = 0; i < 8; i++){
			if((AssistRoad >> (i * 4)) & 0x0F)
				j++;
		}
		*buf++ = HexToChar[j];
		
		if(InterFace == Light_Dim){
			*buf++ = HexToChar[(wave * 10 + 40) >> 4] ;
			*buf++ = HexToChar[(wave * 10 + 40) & 0x0F];
		} else if(InterFace == Test_GUI){
			if(wave == 7)
				*buf++ = '0';
			else if(wave == 8)
				*buf++ = '1';	
		}
			
		sprintf(buf, "%X%X%X", Loop, MainRoad, AssistRoad);
		
	} else if(ParamOrAddr == 1){
		
		if(HexSwitchDec){
			sprintf(tmp, "%04X", ZigBAddr);
		} else {
			sprintf(tmp, "%04d", ZigBAddr);
		}
		sprintf(buf, "B000");
		
		buf += 4;
		if(InterFace == Light_Dim){
			*buf++ = HexToChar[(wave * 10 + 40) >> 4] ;
			*buf++ = HexToChar[(wave * 10 + 40) & 0x0F];
		} else if(InterFace == Test_GUI){
			if(wave == 7)
				*buf++ = '0';
			else if(wave == 8)
				*buf++ = '1';	
		}
	
		sprintf(buf, "%s", tmp);
	}
	
	p = DataSendToBSN((unsigned char *)ctl, (unsigned char *)"FFFF", ret, &len);
	CommxTaskSendData(p, len);
	vPortFree(p);
	
}

static char CentralNode = 0;                  //����������Ľڵ����������0Ϊδ�䣬1Ϊ����  
char StartRead = 0;                           //1Ϊ��ʼ��ȡ����������

void __handleOpenOption(void){                 //��ֵ����TFT��ʾ
	unsigned char dat;
	unsigned char tmp[2];
	
  ChooseLine();	
	
	if(OptDecide == 0)                //����Ƿ�����Ҫ�����ȷ�ϼ����£�Ȼ��ı���ʾ����ֵ
		return;
	
	OptDecide = 0;                    //ȡ��ȷ��
	
	dat = wave;

	if(InterFace == Main_GUI){                   //����ʾҳ��Ϊ���˵�ʱ����ֵ�����ı�ҳ��
		if(dat == 1){
			InterFace = Project_Dir;
			
		} else if(dat == 2){
			InterFace = Config_GUI;
			
		} else if(dat == 3){
			InterFace = Service_GUI;
			
		} else if(dat == 4){
			InterFace = Test_GUI;
			
		} else if(dat == 5){
			InterFace = Intro_GUI;
			
		}
		
	} else if (InterFace == Config_GUI){         //����ʾҳ��Ϊ���ý���ʱ����ֵ�����ı�ҳ��
		
		if(dat == 1){
			
			InterFace = GateWay_Set;
		} else if(dat == 2){
			if(NumOfFrequ > 1){
				InterFace = Frequ_Set;
			}else
				return;
			
		} else if(dat == 3){
			InterFace = Address_Set;
			__AddrConfig();
		} else if(dat == 4){
			InterFace = Config_Set;
		
		} else if(dat == 5){
			InterFace = Config_DIS;
			Ili9320TaskClear("C", 2);			
		}
		
	} else if (InterFace == Service_GUI){        //����ʾҳ��Ϊά�޽���ʱ����ֵ�����ı�ҳ��

		if(dat == 1){
			
			InterFace = GateWay_Choose;
		} else if(dat == 3){
			InterFace = Address_Choose;
			__AddrConfig();
		} else if(dat == 2){
			if(NumOfFrequ > 1){
				InterFace = Frequ_Choose;
			} else
				return;
			
		} else if(dat == 4){
			char buf[5];
			
			InterFace = Map_Dis;
			if(HexSwitchDec)
				sprintf(buf, "%04X", ZigBAddr);
			else
				sprintf(buf, "%04d", ZigBAddr);
			
			SDTaskHandleSurePosition(buf, strlen(buf));
		} else if(dat == 5){
			InterFace = Lamp_Pole;
			
		} else if(dat == 7){
			InterFace = Read_Data;
			Ili9320TaskClear("C", 2);
			StartRead = 1;
			
			return;
		} else if(dat == 8){
			char buf[40] = {0xFF, 0xFF, 0x02, 0x46, 0x46, 0x46, 0x46, 0x30, 0x35, 0x30, 0x35, 0x41, 0x30, 0x30, 0x30, 0x31, 0x34, 0x32, 0x03, 0};    //���ƹص�ָ��
			char buf1[5];
				
			if(HexSwitchDec){
				buf[0] = (ZigBAddr >> 8) & 0xFF;
				buf[1] = ZigBAddr & 0xFF;
				sprintf(buf1, "%04X", ZigBAddr);
			} else {
				buf[0] = ((ZigBAddr/1000) << 4) | (ZigBAddr/100%10);
				buf[1] = ((ZigBAddr/10%10) << 4) | (ZigBAddr%10);
				sprintf(buf1, "%04d", ZigBAddr);
			}
				
			CommxTaskSendData(buf, 19);		
			Line = 10;
			
			sprintf(buf, "��ַ:%s���ص�ָ���ѷ�����", buf1);
			Ili9320TaskOrderDis(buf, strlen(buf) + 1);
			
			return;
		} else if(dat == 9){
			char buf[40] = {0xFF, 0xFF, 0x02, 0x46, 0x46, 0x46, 0x46, 0x30, 0x35, 0x30, 0x35, 0x41, 0x30, 0x30, 0x30, 0x30, 0x34, 0x33, 0x03, 0};    //���ƿ���ָ��
			char buf1[5];
				
			if(HexSwitchDec){
				buf[0] = (ZigBAddr >> 8) & 0xFF;
				buf[1] = ZigBAddr & 0xFF;
				sprintf(buf1, "%04X", ZigBAddr);
			} else {
				buf[0] = ((ZigBAddr/1000) << 4) | (ZigBAddr/100%10);
				buf[1] = ((ZigBAddr/10%10) << 4) | (ZigBAddr%10);
				sprintf(buf1, "%04d", ZigBAddr);
			}
				
			CommxTaskSendData(buf, 19);
			
			Line = 10;
			sprintf(buf, "��ַ:%s������ָ���ѷ�����", buf1);
			Ili9320TaskOrderDis(buf, strlen(buf) + 1);
			
			return;
		}
	} else if (InterFace == Test_GUI){           //����ʾҳ��Ϊ���Խ���ʱ����ֵ�����ı�ҳ��
		
		if(dat == 1){
			
			InterFace = GateWay_Decide;
		} else if(dat == 3){
			InterFace = Address_Option;
			ParamOrAddr = 1;
			Loop = 0;        //��·
			MainRoad = 0;    //����
			AssistRoad = 0;  //����
			Flood = 0;      //Ͷ��
			__AddrConfig();
		} else if(dat == 2){
			if(NumOfFrequ > 1){
				InterFace = Frequ_Option;
				
			} else
				return;
			
		} else if(dat == 4){
			InterFace = Real_MOnitor;
			
			SDTaskHandleOpenMap("1", 1);
		} else if(dat == 5){
			char tmp[2] = {1, 0};
			
			InterFace = Light_Attrib;
			Ili9320TaskClear("C", 2);
			Line = 1;
			LightParamDisplay();
			MaxPage = 1;           //ҳ��
			Ili9320TaskLightLine(tmp, 1);
		} else if(dat == 6){
			InterFace = Light_Dim;
			
		} else if(dat == 7){
			char ret[32], tmp[5];
			
			ParamDimm("05");
			
			if(HexSwitchDec){
				sprintf(tmp, "%04X", ZigBAddr);
			} else {
				sprintf(tmp, "%04d", ZigBAddr);
			}
			if(ParamOrAddr == 1)
				sprintf(ret, "��ַ: %s������ָ���ѷ�����", tmp);
			else if(ParamOrAddr == 2)
				sprintf(ret, "����(���ݵƲ�)ָ���ѷ�����");
			Line = 10;
			Ili9320TaskOrderDis(ret, strlen(ret) + 1);
			
			return;
		} else if(dat == 8){
			char ret[32], tmp[5];
			
			ParamDimm("05");
			
			if(HexSwitchDec){
				sprintf(tmp, "%04X", ZigBAddr);
			} else {
				sprintf(tmp, "%04d", ZigBAddr);
			}
			
			if(ParamOrAddr == 1)
				sprintf(ret, "��ַ: %s���ص�ָ���ѷ�����", tmp);
			else if(ParamOrAddr == 2)
				sprintf(ret, "�ص�(���ݵƲ�)ָ���ѷ�����");
			Line = 10;
			Ili9320TaskOrderDis(ret, strlen(ret) + 1);
		
			return;
		} else if(dat == 9){
			InterFace = Listen_Into;		
		}
		
	} else if (InterFace == Project_Dir){            //����ʾҳ��Ϊ��Ŀѡ�����ʱ����ֵ�����ı�ҳ��
		
		if(dat == 1){
			Project = Pro_BinHu;
			
		} else if(dat == 2){
			Project = Pro_ChanYeYuan;
			
		} else if(dat == 3){
			Project = Pro_DaMing;
			
		} else if(dat == 4){
			Project = Pro_YaoHai;
			
		} else if(dat == 5){
			Project = Pro_JingKai;
			
		}
	
		tmp[0] = Open_GUI;                              /*������һ������ʾ���˵�*/
	  tmp[1] = KEYMENU;	
		SDTaskHandleKey((const char *)tmp, 2);
		InterFace = Main_GUI;
	}  else if (InterFace == GateWay_Set){            //����ʾҳ��Ϊ����ѡ�����ʱ����ֵ�����ı�ҳ��   

		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
		
		tmp[0] = Main_GUI;                              /*������һ������ʾ���ý���*/
	  tmp[1] = 2;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Config_GUI;
		
	} else if(InterFace == GateWay_Choose) {           //����ʾҳ��Ϊ����ѡ�����ʱ����ֵ�����ı�ҳ��   
		
		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
	
		tmp[0] = Main_GUI;                               /*������һ������ʾά�޽���*/
	  tmp[1] = 3;
	  SDTaskHandleKey((const char *)tmp, 2);

		InterFace = Service_GUI;
		
	} else if(InterFace == GateWay_Decide) {           //����ʾҳ��Ϊ����ѡ�����ʱ����ֵ�����ı�ҳ��   

		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
		
		tmp[0] = Main_GUI;                                /*������һ������ʾ���Խ���*/
	  tmp[1] = 4;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Test_GUI;
	} else if(InterFace == Frequ_Set) {                 //����ʾҳ��ΪƵ��ѡ�����ʱ����ֵ����

		
		tmp[0] = Main_GUI;                                /*������һ������ʾ���ý���*/
	  tmp[1] = 2;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Config_GUI;
		
	} else if(InterFace == Frequ_Choose) {              //����ʾҳ��ΪƵ��ѡ�����ʱ����ֵ����

		tmp[0] = Main_GUI;															  /*������һ������ʾά�޽���*/
	  tmp[1] = 3;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Service_GUI;
		
	} else if(InterFace == Frequ_Option) {              //����ʾҳ��ΪƵ��ѡ�����ʱ����ֵ����
			
		tmp[0] = Main_GUI;														    /*������һ������ʾ���Խ���*/
	  tmp[1] = 4;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Test_GUI;
	} else if(InterFace == Config_DIS){
		Config_Enable = 1;
		
		ConfigTaskSendData("1\r\n", 4);
		Ili9320TaskClear("C", 2);
	} else if(InterFace == Address_Choose){
		
		ComxTaskRecieveModifyData("1\r\n", 4);
		Ili9320TaskClear("C", 2);
		
	}  else if(InterFace == Read_Data){
		Ili9320TaskClear("C", 2);
		StartRead = 1;
	} else if(InterFace == Light_Dim) {
		char ret[32];
		
		if(wave == 1){
			DimmValue = 50;
		} else if(wave == 2){
			DimmValue = 60;
		} else if(wave == 3){
			DimmValue = 70;
		} else if(wave == 4){
			DimmValue = 80;
		} else if(wave == 5){
			DimmValue = 90;
		} else if(wave == 6){
			DimmValue = 100;
		}
		ParamDimm("04");
		
		sprintf(ret, "%d%%����ָ���ѷ�����", DimmValue);
		Line = 10;
		Ili9320TaskOrderDis(ret, strlen(ret) + 1);
		return;
	} else if(InterFace == Lamp_Pole){
		if(wave == 1){
			char buf[40];
			
			SDTaskHandleLightPole("1", 1);
			
			Line = 10;
			sprintf(buf, "�رյ�ǰ�˵�ָ���ѷ�����");
			Ili9320TaskOrderDis(buf, strlen(buf) + 1);
		} else if(wave == 2){
			char buf[40];
			
			SDTaskHandleLightPole("3", 1);
			
			Line = 10;
			sprintf(buf, "�ر��ڽ��˵�ָ���ѷ�����");
			Ili9320TaskOrderDis(buf, strlen(buf) + 1);
			
		} else if(wave == 3){
			char buf[40] = {0xFF, 0xFF, 0x02, 0x46, 0x46, 0x46, 0x46, 0x30, 0x35, 0x30, 0x35, 0x41, 0x30, 0x30, 0x30, 0x30, 0x34, 0x33, 0x03, 0};    //���ƿ���ָ��
		
			CommxTaskSendData(buf, 19);
			
			Line = 10;
			sprintf(buf, "����ָ���ѷ�����");
			Ili9320TaskOrderDis(buf, strlen(buf) + 1);
		} else if(wave == 4){
			
			SDTaskHandleAddress("1", 1);	
		}
		
		return;
	} else if(InterFace == Listen_Into){
		
		if(wave == 2){
			char *p, buf[32];
			unsigned char len;
			
			if(CentralNode == 0)
				return;
			
			if(FrequencyDot == 1)
				sprintf(buf, "%X%02X", FrequPoint1, NetID1);
			if(FrequencyDot == 2)
				sprintf(buf, "%X%02X", FrequPoint2, NetID2);
			
			p = DataSendToBSN((unsigned char *)"AA", (unsigned char *)"0AAA", buf, &len);
			CommxTaskSendData(p, len);
			vPortFree(p);
			
			Line = 10;
			sprintf(buf, "���ü�����ָ���ѷ�����");
			Ili9320TaskOrderDis(buf, strlen(buf) + 1);
		} else if(wave == 1){
			
			
			 CentralNode = 1;
		}
	}
		
	wave = 1;                                            //ҳ���л���������һ��
}

void AttributeDisplay(void){
	char buf[40];
	
	if(wave == 1){
		Line = 1;
		if(Loop)
			sprintf(buf, " ��·ѡ��:  %-8X", Loop);
		else
			sprintf(buf, " ��·ѡ��:");	
		Ili9320TaskOrderDis(buf, strlen(buf) + 1);
		
	} else if(wave == 2){
		Line = 2;
		if(MainRoad)
			sprintf(buf, " ����ѡ��:  %-8X", MainRoad);		
		else
			sprintf(buf, " ����ѡ��:");
		Ili9320TaskOrderDis(buf, strlen(buf) + 1);
		
	} else if(wave == 3){
		Line = 3;
		if(AssistRoad)
			sprintf(buf, " ����ѡ��:  %-8X", AssistRoad);	
		else
			sprintf(buf, " ����ѡ��:");
		Ili9320TaskOrderDis(buf, strlen(buf) + 1);
		
	} else if(wave == 4){
		Line = 4;
		if(Flood)
			sprintf(buf, " Ͷ��ѡ��:  ����   ����");
		else
			sprintf(buf, " Ͷ��ѡ��:  ����   ����");
		Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	}
}

void __handleSetAttribute(void){
	char tmp[2] = {1, 0};

	if(KeyConfirm == KEYUP){
		wave--;
		if(wave < 1)
			wave = MaxLine;	
	} else if (KeyConfirm == KEYDN){
		wave++;
		if(wave > MaxLine)		
			wave = 1;
	} else if(KeyConfirm == KEYLF){
		if(wave == 4){
			Flood = 1;	
		} else if(wave == 1){
			Loop >>= 4;
		}	else if(wave == 2){
			MainRoad >>= 4;
		}	else if(wave == 3){
			AssistRoad >>= 4;
		}	
		
	 } else if(KeyConfirm == KEYRT){
		 if(wave == 4)
			 Flood = 0;	
	
	 } else if(KeyConfirm == KEYOK){
		InterFace = Test_GUI;
		
		dat[0] = Main_GUI;
	  dat[1] = 4;
	  SDTaskHandleKey((const char *)dat, 2);
		
		wave = 1;
		return;
	} else if((KeyConfirm > KEY0) && (KeyConfirm < KEY9)){
		if(wave == 1){
			if((Loop & 0xF) < (KeyConfirm - 1))
				Loop = (Loop << 4) | (KeyConfirm - 1);
		} else if(wave == 2){
			if((MainRoad & 0xF) < (KeyConfirm - 1))
				MainRoad = (MainRoad << 4) | (KeyConfirm - 1);
		} else if(wave == 3){
			if((AssistRoad & 0xF) < (KeyConfirm - 1))
				AssistRoad = (AssistRoad << 4) | (KeyConfirm - 1);
		}

	}
	
	AttributeDisplay();

	tmp[0] = wave;
	Ili9320TaskLightLine(tmp, 1);
}

void DisplayAttribute(void){
	char buf[80], tmp[10], i;
	
	if(Flood)
		sprintf(tmp, "��ѡ��");
	else
		sprintf(tmp, "δѡ��");
	
	if((Loop != 0) || (MainRoad != 0) || (AssistRoad != 0) || (Flood != 0)){
		ParamOrAddr = 2;
		ZigBAddr = 1;
		sprintf(buf, "��·ѡ��: %-8X  ����ѡ��: %-8X  ����ѡ��: %-8X  Ͷ���  : %s", Loop, MainRoad, AssistRoad, tmp);	
	} else {
		return;
	}
	
	if(Loop == 0){
		for(i = 0; i < 8; i++)
			buf[10 + i] = ' ';
	} 
	
	if(MainRoad == 0){
		for(i = 0; i < 8; i++)
			buf[30 + i] = ' ';
	}
	
	if(AssistRoad == 0){
		for(i = 0; i < 8; i++)
			buf[50 + i] = ' ';
	}
	
	Ili9320TaskDisLightParam(buf, strlen(buf) + 1);
}

void __DisplayWGInformation(void){                 //��ʾ��Ŀ�����ء�Ƶ�����Ϣ
	if(times%40 == 0){
		DisplayInformation();
	}
}

void __DisplayProject(void){                       //������ʱ����ʾ��Ŀ
	if(times%40 == 0){
		DisplayProjet();
	}
}

void __DisplayNode(void){                       //��ʾ���Ľڵ��Ƶ�������ID
	if(times%40 == 0){
		DisplayNode();
	}
}

void __DisplayLightPara(void){
	if(times%40 == 0)
		DisplayAttribute();
}

void __handleSwitchInput(void){                   //1~7���ڡ�1~7���롰A~L���������л�
	if(KeyConfirm == KEYINPT){
		if(IME == 0)
			IME = 1;
		else
			IME = 0;

	}
	
	if(times%30 == 0){
		InputChange();
	}
		
	if(IME == 1){
		if(KeyConfirm == KEY1){
			KeyConfirm = KEYA;
		} else if (KeyConfirm == KEY2){
			KeyConfirm = KEYB;
		} else if (KeyConfirm == KEY3){
			KeyConfirm = KEYC;
		} else if (KeyConfirm == KEY4){
			KeyConfirm = KEYD;
		} else if (KeyConfirm == KEY5){
			KeyConfirm = KEYE;
		} else if (KeyConfirm == KEY6){
			KeyConfirm = KEYF;
		}	else if (KeyConfirm == KEY7){
			if(InterFace == Config_Set)
				KeyConfirm = KEYL;
			else 
				KeyConfirm = KEY7;
		}
	}	
}


void __handleBSNData(void){                           //��������������� 
	const char *buf = "\r\n���ڶ�ȡ���������ݣ����Ժ�... ...";
	const char *ctl = "06";
	char *p, tmp[5];
	unsigned char len;
	
	if(HexSwitchDec){
		sprintf(tmp, "%04X", ZigBAddr);
	} else {
		sprintf(tmp, "%04d", ZigBAddr);
	}
	
	p = DataSendToBSN((unsigned char *)ctl, (unsigned char *)tmp, NULL, &len);
	StartRead = 1;
	CommxTaskSendData(p, len);
	vPortFree(p);
	
	Line = 1;
	Ili9320TaskOrderDis(buf, strlen(buf) + 1);
}

extern char NumberOfMap(void);                       //����ͼ��Ŀ
static char Curr = 1;                                //��ǰ��ʾ�ĵ�ͼ���

void __handleRealTime(void){                         //����ʵʱ��ؽ���
	char Tol, buf[2];
	
	Tol = NumberOfMap();
	
	if(KeyConfirm == KEYUP){
		Curr--;
		if(Curr < 1)
			Curr = 1;	
	} else if(KeyConfirm == KEYDN){
		Curr++;
		if(Curr > Tol)
			Curr = Tol;
	}
	
	sprintf(buf, "%d", Curr);
	
	SDTaskHandleOpenMap(buf, 2);
}

bool HexSwitchDec = 0;                               //16������10�����л��� 0λ10���ƣ�1Ϊ16����
static char Congif_Flag = 0;                         //�Ƿ����������ü�

void __handleAddrValue(void){	                            //����ZigBee��ַ����
	char buf[36], dat[2];
	
	if(Congif_Flag){
		Ili9320TaskClear("C", 2);
		Congif_Flag = 0;
	}
	
	if(KeyConfirm == KEYLF){
		Digits--;
		if(Digits < 1)
			Digits = MaxBit;
	} else if(KeyConfirm == KEYRT){
		Digits++;
		if(Digits > MaxBit)
			Digits = 1;
	} else if(KeyConfirm == KEYUP){
		ZigBAddr--;
		if(ZigBAddr < 1)
			ZigBAddr = 1;
	} else if(KeyConfirm == KEYDN){
		if(HexSwitchDec){
			if((ZigBAddr & 0xFFF) == 0xFFF)
				ZigBAddr = ZigBAddr & 0xF001;		
		} else{
			if((ZigBAddr % 1000) == 999)
				ZigBAddr = ZigBAddr / 1000 * 1000 + 1;
		}
		ZigBAddr++;
	} else if(KeyConfirm == KEYOK){
		if(InterFace == Address_Choose){
			InterFace = Service_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 3;
	    SDTaskHandleKey((const char *)dat, 2);
			
			SDTaskHandleLampParam("1", 1);
		} else if(InterFace == Address_Option){
			InterFace = Test_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 4;
	    SDTaskHandleKey((const char *)dat, 2);
		} else {
			Congif_Flag = 1;
			Config_Enable = 1;
			ConfigTaskSendData("1", 2);
			Ili9320TaskClear("C", 2);
		}
	}
	
	if((KeyConfirm >= KEY0) && (KeyConfirm <= KEYF)){		
		if(HexSwitchDec){
			ZigBAddr &= ~(0xF << ((4 - Digits) * 4));                //����Ҫ�ı��λ����0
			ZigBAddr |= ((KeyConfirm - 1) << ((4 - Digits) * 4));    //�޸ĵ�ֵַΪ����ļ�ֵ
		} else{	
			ZigBAddr = ZigBAddr / (int)pow(10, (5 - Digits)) * (int)pow(10, (5 - Digits)) + ZigBAddr % (int)pow(10, (4 - Digits)) + (KeyConfirm - 1) * pow(10, (4 - Digits));			
		}
		
		Digits++;
			if(Digits > MaxBit)
				Digits = 1;
	}
	
	if(HexSwitchDec){
		if(!(ZigBAddr & 0x0FFF))
			ZigBAddr |= 0x0001;
		if(ZigBAddr > 0xFFFF)
			ZigBAddr = 1;
		sprintf(buf, "%04X", ZigBAddr);
	} else {
		if(!(ZigBAddr % 1000))
			ZigBAddr += 1;
		if(ZigBAddr > 9999)
			ZigBAddr = 1;
		sprintf(buf, "%04d", ZigBAddr);
	}
	buf[4] = 0;
	
	Line = 2;
	
	if(lastData != ZigBAddr){
		Ili9320TaskClear("1", 2);
		Ili9320TaskOrderDis(buf, strlen(buf) + 1);
	}
	
	buf[0] = Digits + BaseBit;
	buf[1] = 0;
	Ili9320TaskLightByte(buf, strlen(buf) + 1);
}

void TIM3_IRQHandler(void){	
	unsigned char dat[2];
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); 
		times++;
//		if(times == 100)
//			SDTaskHandleLampParam("1", 1);
		if(times > 120)
			times = 1;
	}
	
	key_driver(keycode()); 
	
	if(InterFace == Main_GUI){
		__DisplayProject();
	}
	
	if((InterFace == Service_GUI) || (InterFace == Test_GUI) || (InterFace == Address_Option) || (InterFace == Light_Dim)){
		__DisplayNode();
	}
	
	if((InterFace == Test_GUI) || (InterFace == Light_Dim)) {
		__DisplayLightPara();
	}
	
	if((InterFace == Lamp_Pole) || (InterFace == Config_GUI) || (InterFace == Frequ_Set) || (InterFace == Address_Set) || 
		(InterFace == Service_GUI) || (InterFace == Address_Choose) || (InterFace == Test_GUI) || (InterFace == Address_Option) || 
		(InterFace == Light_Dim))
	
		__DisplayWGInformation();
	
	if((InterFace == Config_Set) || (InterFace == Address_Set) || (InterFace == Config_Set)|| (InterFace == Address_Choose))
		__handleSwitchInput();
	
	if(KeyConfirm == NOKEY)
		return;
	
	if(((InterFace == Service_GUI) || (InterFace == Test_GUI))&& (KeyConfirm == KEYCONF) && (FrequencyDot != 0)){
		HubNode = 2;                                          //����ά�޽���� ��ʼ�������Ľڵ�
		CommxTaskSendData("1", 1);
		
		if(FrequencyDot == 1){
			NodeFrequ = FrequPoint1;
			NodeID = NetID1;
		} else if(FrequencyDot == 2){
			NodeFrequ = FrequPoint2;
			NodeID = NetID2;
		}
		
		KeyConfirm = NOKEY;
		return;
	}		
		
	if((InterFace != Address_Set) && (InterFace != Config_DIS))
		Config_Enable = 0;
	
	if(KeyConfirm == KEYMENU){
		
	  dat[0] = Open_GUI;
	  dat[1] = KEYMENU;
	  SDTaskHandleKey((const char *)dat, 2);
	  InterFace = Main_GUI;
		KeyConfirm = NOKEY;
	  return;
  } else if(KeyConfirm == KEYRETURN){
		if((InterFace == Project_Dir) || (InterFace == Config_GUI) || (InterFace == Service_GUI) || (InterFace == Test_GUI) || 
			(InterFace == Intro_GUI)){
			InterFace = Main_GUI;
			
	  	dat[0] = Open_GUI;
	    dat[1] = KEYMENU;
	    SDTaskHandleKey((const char *)dat, 2);
		
		} else if(InterFace == GateWay_Set || InterFace == Address_Set || InterFace == Config_Set 
			|| InterFace == Config_DIS || InterFace == Frequ_Set){
			InterFace = Config_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 2;
	    SDTaskHandleKey((const char *)dat, 2);
		} else if(InterFace == GateWay_Choose || InterFace == Lamp_Pole || 
			InterFace == Address_Choose || InterFace == Read_Data || InterFace == Frequ_Choose || InterFace == Map_Dis){
				
			if(InterFace == Address_Choose)	
				SDTaskHandleLampParam("1", 1);
			
			if(InterFace == Read_Data)
				SDTaskHandleCloseFile("1", 1);
			
			InterFace = Service_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 3;
	    SDTaskHandleKey((const char *)dat, 2);		
			
		} else if((InterFace == GateWay_Decide) || (InterFace == Address_Option) || (InterFace == Light_Dim) || 
			(InterFace == Light_Attrib) || (InterFace == Frequ_Option) || (InterFace == Real_MOnitor)){
				
			if(InterFace == Real_MOnitor){
				HubNode = 0;
				StartRead = 0;
			}
			InterFace = Test_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 4;
	    SDTaskHandleKey((const char *)dat, 2);
			
		} 
		
		if(InterFace != Main_GUI){
			wave = 1;
			page = 1;
	  }
		KeyConfirm = NOKEY;
	}
	
	if(LastFace != InterFace){
		LastFace = InterFace;
		page = 1;
	}
	
	ProMaxPage();
	
	if(InterFace == Main_GUI){

		__handleOpenOption();	
	} else if(InterFace == Config_GUI){

		__handleOpenOption();
	} else if(InterFace == Service_GUI){

		__handleOpenOption();
	} else if(InterFace == Test_GUI){

		__handleOpenOption();
	} else if(InterFace == Intro_GUI){

		__handleOpenOption();
	} else if(InterFace == Project_Dir){

		__handleOpenOption();
	} else if(InterFace == Light_Dim){

		__handleOpenOption();
	} else if(InterFace == Config_Set){
		
		__handleAdvanceSet();		
	} else if(InterFace == GateWay_Set){
		
		__handleOpenOption();
	} else if(InterFace == GateWay_Choose){
		
		__handleOpenOption();
	} else if(InterFace == GateWay_Decide){
		
		__handleOpenOption();
	} else if(InterFace == Frequ_Set){

		__handleOpenOption();
	} else if(InterFace == Frequ_Choose){

		__handleOpenOption();
	} else if(InterFace == Frequ_Option){

		__handleOpenOption();
	} else if(InterFace == Address_Set){

		__handleAddrValue();
	} else if(InterFace == Config_DIS){

		__handleOpenOption();
	} else if(InterFace == Address_Choose){
	
		__handleAddrValue();
	} else if(InterFace == Read_Data){
	
		__handleBSNData();
	} else if(InterFace == Light_Attrib){
		
		__handleSetAttribute();
	} else if(InterFace == Address_Option){
	
		__handleAddrValue();
	} else if(InterFace == Real_MOnitor){
		
		__handleRealTime();
	} else if(InterFace == Lamp_Pole){
		
		__handleOpenOption();
	}
	
	KeyConfirm = NOKEY;
}

void __HandleConfigKey(KeyTaskMsg *dat){
	char *p =__KeyGetMsgData(dat);
	ConfigTaskSendData(p, strlen(p));
}

typedef struct {
	KeyTaskMsgType type;
	void (*handlerFunc)(KeyTaskMsg *);
} MessageHandlerMap;

static const MessageHandlerMap __messageHandlerMaps[] = {
	{ KEY_SEND_DATA, __HandleConfigKey },
	{ KEY_NULL, NULL },
};

static void __KeyTask(void *parameter) {
	portBASE_TYPE rc;
	KeyTaskMsg *message;

	for (;;) {
	//	printf("Key: loop again\n");
		rc = xQueueReceive(__KeyQueue, &message, configTICK_RATE_HZ);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			for (; map->type != KEY_NULL; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			vPortFree(message);
		} 
	}
}


void KeyInit(void) {
	key_gpio_init();
	TIM3_Init();
	__KeyQueue = xQueueCreate(5, sizeof(KeyTaskMsg *));
	xTaskCreate(__KeyTask, (signed portCHAR *) "KEY", KEY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 4, NULL);
}

