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

#define KEY_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 512)

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
#define KEY_INPUT    GPIO_ReadInputDataBit(GPIO_KEY_1,Pin_Key_1)
#define KEY_0        GPIO_ReadInputDataBit(GPIO_KEY_2,Pin_Key_2)
#define KEY_DIS      GPIO_ReadInputDataBit(GPIO_KEY_3,Pin_Key_3)
#define KEY_DN       GPIO_ReadInputDataBit(GPIO_KEY_4,Pin_Key_4)
#define KEY_2        GPIO_ReadInputDataBit(GPIO_KEY_5,Pin_Key_5)
#define KEY_LF       GPIO_ReadInputDataBit(GPIO_KEY_DEL,Pin_Key_DEL)
#define KEY_RT       GPIO_ReadInputDataBit(GPIO_KEY_OK,Pin_Key_OK)
#define KEY_MENU     GPIO_ReadInputDataBit(GPIO_KEY_MENU,Pin_Key_MENU)
#define KEY_OK       GPIO_ReadInputDataBit(GPIO_KEY_LF,Pin_Key_LF)
#define KEY_3        GPIO_ReadInputDataBit(GPIO_KEY_INPT,Pin_Key_INPT)
#define KEY_UP       GPIO_ReadInputDataBit(GPIO_KEY_MODE,Pin_Key_MODE)
#define KEY_RET      GPIO_ReadInputDataBit(GPIO_KEY_RT,Pin_Key_RT)

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

KeyPress keycode(void){                 //检测有变化按键，需被确认
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
	} else if(KEY_DIS == 0){
		return KEYDIS;
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

static KeyPress KeyConfirm = NOKEY;         //上电后按键的初始状态

static Dis_Type  InterFace = Open_GUI;      //上电后显示界面的初始状态

void key_driver(KeyPress code)              //检测有效按键
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

static void InputChange(void){                   //显示当前输入法
	char tmp[5];
	if(IME == 0)
			strcpy(tmp, "123");
		else
			strcpy(tmp, "ABC");
		Ili9320TaskInputDis(tmp, strlen(tmp) + 1);
}

static char times = 0;             //定时器中断计数
static char count = 0;             //输入数字键的个数
static char dat[9];                //输入数字键组合

extern void ili9320_SetLight(char line);

static unsigned char wave = 1;     //行数
static unsigned char page = 1;     //页数

static unsigned char MaxPage = 1;  //最大页数

static unsigned char OptDecide = 0; //确定选项

bool DisStatus(char type){              //判断Dis_Type类型下哪种类型值在按下确认键后，需要被单独处理
	switch (type){
		case 2:
			return true;
		case 3:
			return true;
		case 4:
			return true;
		case 5:
			return true;
		case 6:
			return true;
		case 7:
			return true;
		case 8:
			return true;
		case 12:
			return true;
		case 15:
			return true;
		case 18:
			return true;
		default:
			return false;
	}	
}

void ChooseLine(void){                         //确定行数，和页数
	unsigned char tmp[2];
	
	if(KeyConfirm == KEYUP){
		wave--;
	} else if(KeyConfirm == KEYDN){
		wave++;
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
		
		if(DisStatus(InterFace)){
					
			tmp[0] = InterFace;
			tmp[1] = wave;
			SDTaskHandleKey((const char *)tmp, 2);
			OptDecide = 1;
		}
						
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
	
	ili9320_SetLight(wave);
}


	
void __handleAdvanceSet(void){                          //高级配置类型下，按键处理函数
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


static pro Project = Pro_Null;                      //初始化项目为无
static unsigned char FrequencyDot = 0;              //初始频点为无

pro DeterProject(void){
	return Project;
}

extern unsigned char *GWname(void);

extern char NumberOfPoint(void);

void DisplayInformation(void){                       //显示选择的项目，网关，频点等
	char buf[40], tmp[6];
	
	if(NumberOfPoint() == 1){
		FrequencyDot = 1;
	}
	
  if(FrequencyDot == 1){
		sprintf(tmp, "/频点1");
	} else if(FrequencyDot == 2){
		sprintf(tmp, "/频点2");
	} else {
		tmp[0] = 0;
	}
	
	if(Project == Pro_BinHu){
		sprintf(buf, "%s%s%s", "滨湖/", GWname(), tmp);
	} else if(Project == Pro_ChanYeYuan){
		sprintf(buf, "%s%s%s", "产业园/", GWname(), tmp);
	} else if(Project == Pro_DaMing){
		sprintf(buf, "%s%s%s", "公司/", GWname(), tmp);
	}
	
	Ili9320TaskDisGateWay(buf, strlen(buf) + 1);
}

void __handleOpenOption(void){                 //键值操作TFT显示
	unsigned char dat;
	unsigned char tmp[2];
	
  ChooseLine();	
	
	if(OptDecide == 0)                //检测是否有需要处理的确认键按下
		return;
	
	OptDecide = 0;                    //取消确认
	
	dat = wave;

	if(InterFace == Main_GUI){                   //当显示页面为主菜单时，键值操作改变页面
		if(dat == 1){
			InterFace = Project_Dir;
		} else if(dat == 2)
			InterFace = Config_GUI;
		else if(dat == 3)
			InterFace = Service_GUI;
		else if(dat == 4)
			InterFace = Test_GUI;
		else if(dat == 5)
			InterFace = Intro_GUI;
	} else if (InterFace == Config_GUI){         //当显示页面为配置界面时，键值操作改变页面
		if(dat == 1)
			InterFace = GateWay_Set;
		else if(dat == 2)
			InterFace = Address_Set;
		else if(dat == 3){
			if(NumberOfPoint() > 1)
				InterFace = Frequ_Set;
			else
				return;
		} else if(dat == 4)
			InterFace = Config_Set;
		else if(dat == 5)
			InterFace = Config_DIS;
		
	} else if (InterFace == Service_GUI){        //当显示页面为维修界面时，键值操作改变页面
		if(dat == 1)
			InterFace = GateWay_Choose;
		else if(dat == 2)
			InterFace = Address_Choose;
		else if(dat == 3){
			if(NumberOfPoint() > 1)
				InterFace = Frequ_Choose;
			else
				return;
		} else if(dat == 4)
			InterFace = Read_Data;
		
	} else if (InterFace == Test_GUI){           //当显示页面为测试界面时，键值操作改变页面
		if(dat == 1)
			InterFace = GateWay_Decide;
		else if(dat == 2)
			InterFace = Address_Option;
		else if(dat == 3){
			if(NumberOfPoint() > 1)
				InterFace = Frequ_Option;
			else
				return;
		} else if(dat == 4)
			InterFace = Debug_Option;
		else if(dat == 5)
			InterFace = Light_Dim;
		else if(dat == 6)
			InterFace = On_And_Off;
		
	} else if (InterFace == Project_Dir){            //当显示页面为项目选择界面时，键值操作改变页面
		
		if(dat == 1){
			Project = Pro_BinHu;
		} else if(dat == 2){
			Project = Pro_ChanYeYuan;
		} else if(dat == 3){
			Project = Pro_DaMing;
		}
	
		tmp[0] = Open_GUI;
	  tmp[1] = KEYMENU;	
		SDTaskHandleKey((const char *)tmp, 2);
		InterFace = Main_GUI;
	}  else if (InterFace == GateWay_Set){             //当显示页面为网关选择界面时，键值操作改变页面   
					 
		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
		
		tmp[0] = Main_GUI;
	  tmp[1] = 2;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Config_GUI;
		
	} else if(InterFace == GateWay_Choose) {           //当显示页面为网关选择界面时，键值操作改变页面   
		
		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
	
		tmp[0] = Main_GUI;
	  tmp[1] = 3;
	  SDTaskHandleKey((const char *)tmp, 2);

		InterFace = Service_GUI;
		
	} else if(InterFace == GateWay_Decide) {           //当显示页面为网关选择界面时，键值操作改变页面   
		
		tmp[0] = InterFace;
		tmp[1] = wave;
		SDTaskHandleWGOption((const char *)tmp, 2);	
		
		tmp[0] = Main_GUI;
	  tmp[1] = 4;
	  SDTaskHandleKey((const char *)tmp, 2);
		
		InterFace = Test_GUI;
	}
		
	wave = 1;                                           //页面切换后，显亮第一行
	KeyConfirm = NOKEY;
}

void __handleSwitchInput(void){                       //1~7键在“1~7”与“A~L”间来回切换
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
}

extern unsigned char NumOfPage(void);

unsigned char ProMaxPage(void){              //确认显示类型的最大页数
	MaxPage = NumOfPage();
	return page;
}

static bool U3IRQ_Enable = false;             //使能串口3接收数据

bool Com3IsOK(void){
	if(InterFace == Config_Set)
		U3IRQ_Enable = true;
	else
		U3IRQ_Enable = false;
	return U3IRQ_Enable;
}

void TIM3_IRQHandler(void){	
	unsigned char dat[2];
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); 
		times++;
	}
	
	key_driver(keycode());                  
	
	if(KeyConfirm == NOKEY)
		return;
	
	if(KeyConfirm == KEYDIS)
		DisplayInformation();
	
	if(KeyConfirm == KEYMENU){
		
	  dat[0] = Open_GUI;
	  dat[1] = KEYMENU;
	  SDTaskHandleKey((const char *)dat, 2);
	  InterFace = Main_GUI;
		KeyConfirm = NOKEY;
	  return;
  } else if(KeyConfirm == KEYRETURN){
		if((InterFace == Project_Dir) || (InterFace == Config_GUI) || (InterFace == Service_GUI) || (InterFace == Test_GUI) || (InterFace == Intro_GUI)){
			InterFace = Main_GUI;
			
	  	dat[0] = Open_GUI;
	    dat[1] = KEYMENU;
	    SDTaskHandleKey((const char *)dat, 2);
		
		} else if(InterFace == GateWay_Set || InterFace == Address_Set || InterFace == Config_Set || InterFace == Config_DIS || InterFace == Frequ_Set){
			InterFace = Config_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 2;
	    SDTaskHandleKey((const char *)dat, 2);
		} else if(InterFace == GateWay_Choose || InterFace == Address_Choose || InterFace == Read_Data || InterFace == Frequ_Choose){
			InterFace = Service_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 3;
	    SDTaskHandleKey((const char *)dat, 2);
		} else if(InterFace == GateWay_Decide || InterFace == Address_Option || InterFace == Debug_Option || InterFace == Light_Dim || InterFace == On_And_Off || InterFace == Frequ_Option){
			InterFace = Test_GUI;
			
			dat[0] = Main_GUI;
	    dat[1] = 4;
	    SDTaskHandleKey((const char *)dat, 2);
			
		} 
		
		wave = 1;
		page = 1;
		KeyConfirm = NOKEY;
		return;
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
		__handleSwitchInput();
		__handleAdvanceSet();		
	} else if(InterFace == GateWay_Set){

		__handleOpenOption();
	} else if(InterFace == GateWay_Choose){

		__handleOpenOption();
	} else if(InterFace == GateWay_Decide){
		
		__handleOpenOption();
	} else if(InterFace == Address_Option)
		__handleOpenOption();
	else if(InterFace == Debug_Option)
		__handleOpenOption();
	
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
	xTaskCreate(__KeyTask, (signed portCHAR *) "KEY", KEY_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}

