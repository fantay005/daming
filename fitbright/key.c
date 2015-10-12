#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "key.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "zklib.h"
#include "ili9320.h"
#include "ConfigZigbee.h"
#include "sdcard.h"

#define KEY_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 256)

static xQueueHandle __KeyQueue;

#define KEY_STATE_0    0       //初始状态
#define KEY_STATE_1    1       //消抖确认状态
#define KEY_STATE_2    2       //等待按键释放状态

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
#define Pin_Key_UP     GPIO_Pin_5            //向上键

#define GPIO_KEY_DOWN  GPIOB
#define Pin_Key_DOWN   GPIO_Pin_0            //向下键

#define GPIO_KEY_LF    GPIOB
#define Pin_Key_LF     GPIO_Pin_1            //向左键

#define GPIO_KEY_RT    GPIOB
#define Pin_Key_RT     GPIO_Pin_2            //向右键

#define GPIO_KEY_MODE  GPIOB
#define Pin_Key_MODE   GPIO_Pin_13           //模式键

#define GPIO_KEY_MENU  GPIOB
#define Pin_Key_MENU   GPIO_Pin_14           //功能键

#define GPIO_KEY_INPT  GPIOB
#define Pin_Key_INPT   GPIO_Pin_15           //输入切换键 

#define GPIO_KEY_DEL   GPIOF                         
#define Pin_Key_DEL    GPIO_Pin_7            //删除键

#define GPIO_KEY_OK     GPIOF
#define Pin_Key_OK      GPIO_Pin_6           //确定键

/*上面的定义是针对杜工按键板定义*/

#define KEY_1        GPIO_ReadInputDataBit(GPIO_KEY_6,Pin_Key_6) 
#define KEY_4        GPIO_ReadInputDataBit(GPIO_KEY_7,Pin_Key_7)
#define KEY_5        GPIO_ReadInputDataBit(GPIO_KEY_8,Pin_Key_8)
#define KEY_6        GPIO_ReadInputDataBit(GPIO_KEY_9,Pin_Key_9)
#define KEY_7        GPIO_ReadInputDataBit(GPIO_KEY_UP,Pin_Key_UP)
#define KEY_8        GPIO_ReadInputDataBit(GPIO_KEY_DOWN,Pin_Key_DOWN)
#define KEY_9        GPIO_ReadInputDataBit(GPIO_KEY_0,Pin_Key_0)
#define KEY_CLEAR    GPIO_ReadInputDataBit(GPIO_KEY_1,Pin_Key_1)
#define KEY_0        GPIO_ReadInputDataBit(GPIO_KEY_2,Pin_Key_2)
#define KEY_DEL      GPIO_ReadInputDataBit(GPIO_KEY_3,Pin_Key_3)
#define KEY_DN       GPIO_ReadInputDataBit(GPIO_KEY_4,Pin_Key_4)
#define KEY_2        GPIO_ReadInputDataBit(GPIO_KEY_5,Pin_Key_5)
#define KEY_LF       GPIO_ReadInputDataBit(GPIO_KEY_DEL,Pin_Key_DEL)
#define KEY_RT       GPIO_ReadInputDataBit(GPIO_KEY_OK,Pin_Key_OK)
#define KEY_MENU     GPIO_ReadInputDataBit(GPIO_KEY_MENU,Pin_Key_MENU)
#define KEY_OK       GPIO_ReadInputDataBit(GPIO_KEY_LF,Pin_Key_LF)
#define KEY_3        GPIO_ReadInputDataBit(GPIO_KEY_INPT,Pin_Key_INPT)
#define KEY_UP       GPIO_ReadInputDataBit(GPIO_KEY_MODE,Pin_Key_MODE)
#define KEY_INPUT    GPIO_ReadInputDataBit(GPIO_KEY_RT,Pin_Key_RT)

/*重新对按键进行定义*/


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
	
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//PB3用作普通IO
	
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
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);  
 
	TIM_Cmd(TIM3, ENABLE);  
}

static char IME = 0;                    //字母和数字输入法切换，当为1时，1~6输出为A~F

KeyPress keycode(void){
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
	} else if(KEY_DEL == 0){
		return KEYDEL;
	} else if(KEY_MENU == 0){
		return KEYMENU;
	} else if(KEY_INPUT == 0){
		return KEYINPT;
	} else if(KEY_OK == 0){
		return KEYOK;
	} else if(KEY_CLEAR == 0){
		return KEYCLEAR;
	} else {
		return NOKEY;
	}
}

static KeyPress KeyConfirm = NOKEY;

void key_driver(KeyPress code)
{
	static uint32_t key_state = KEY_STATE_0;
	static KeyPress	code_last = NOKEY;
	

	switch (key_state)
	{
		case KEY_STATE_0: //初始状态
			if (code != NOKEY){
				code_last = code;
				key_state = KEY_STATE_1; //如果有键被按下，记录键值，并进入消抖及长按确认状态
			}
			break;
		case KEY_STATE_1: //消抖确认状态
			if (code != code_last)
				key_state = KEY_STATE_2; //等待按键释放状态
			break;
		case KEY_STATE_2:
			if (code != code_last) {//等到按键释放 ，返回初始状态
				KeyConfirm = code_last; //返回键值
				key_state = KEY_STATE_0;
			}
			break;
	}
}

static void InputChange(void){
	char tmp[5];
	if(IME == 0)
			strcpy(tmp, "123");
		else
			strcpy(tmp, "ABC");
		Ili9320TaskInputDis(tmp, strlen(tmp) + 1);
}

static char times = 0;
static char count = 0;
static char dat[9];

extern void ili9320_SetLight(char line);

static unsigned char wave = 1;    //行数
static unsigned char page = 1;    //页数

void __handleGateway(void){
	if(KeyConfirm == KEYUP){
		wave--;
	} else if(KeyConfirm == KEYDN){
		wave++;
	} else if(KeyConfirm == KEYLF){
		wave = 1;
		page--;
	} else if(KeyConfirm == KEYRT){
		wave = 1;
		page++;
	} else if(KeyConfirm == KEYOK){
		SDTaskSureOption((const char *)(wave + 15 * (page - 1)), 1);
		KeyConfirm = NOKEY;
		return;
	}
	
	if(wave > 15){
		wave -= 15;
	} else if(wave < 1){
		wave += 15;
	}
	
	if(page > 4){
		page -= 4;
	} else if(page < 1){
		page += 4;
	}	
	
	ili9320_SetLight(wave + 15 * (page - 1));
	KeyConfirm = NOKEY;
}	
	
void __handleAdvanceSet(void){
	portBASE_TYPE xHigherPriorityTaskWoken;
	char tmp[12] = {0};
	
	KeyTaskMsg *msg;
	char hex2char[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'L'};

	
	if((KeyConfirm >= KEY0) && (KeyConfirm <= KEYL)){		
		dat[count++] = hex2char[KeyConfirm];
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
  }else if(KeyConfirm == KEYCLEAR){
		strcpy(dat, "Clear");
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
  }else {
		KeyConfirm = NOKEY;
		return;
	}
	
	msg = __KeyCreateMessage(KEY_SEND_DATA, dat, strlen(dat) + 1);
	KeyConfirm = NOKEY;
	
	if(strncasecmp(dat, "Clear", 5) == 0){
		Ili9320TaskClear(dat, strlen(dat));
		memset(dat, 0, 9);
		return;
	}
	Ili9320TaskOrderDis(dat, strlen(dat) + 1);
	if (pdTRUE == xQueueSendFromISR(__KeyQueue, &msg, &xHigherPriorityTaskWoken)) {
		if (xHigherPriorityTaskWoken) {
			portYIELD();
		}
	}
  memset(dat, 0, 9);
} 

void TIM3_IRQHandler(void){	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); 
		times++;
	}
	
	key_driver(keycode());
	
	if(KeyConfirm == KEYINPT){
		if(IME == 0)
			IME = 1;
		else
			IME = 0;
	}
	
	if(times > 40){
		times = 0;
		InputChange();
	}
	
	if(KeyConfirm == NOKEY)
		return;
		
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
			KeyConfirm = KEYL;
		}	
	}
	
	__handleGateway();
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
	xTaskCreate(__KeyTask, (signed portCHAR *) "KEY", KEY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}

