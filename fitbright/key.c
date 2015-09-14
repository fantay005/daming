#include "key.h"
#include "stm32f10x_gpio.h"

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

#define KEY_1        GPIO_ReadInputDataBit(GPIO_KEY_0,Pin_Key_0) 
#define KEY_4        GPIO_ReadInputDataBit(GPIO_KEY_1,Pin_Key_1)
#define KEY_5        GPIO_ReadInputDataBit(GPIO_KEY_2,Pin_Key_2)
#define KEY_6        GPIO_ReadInputDataBit(GPIO_KEY_3,Pin_Key_3)
#define KEY_7        GPIO_ReadInputDataBit(GPIO_KEY_4,Pin_Key_4)
#define KEY_8        GPIO_ReadInputDataBit(GPIO_KEY_5,Pin_Key_5)
#define KEY_9        GPIO_ReadInputDataBit(GPIO_KEY_6,Pin_Key_6)
#define KEY_RET      GPIO_ReadInputDataBit(GPIO_KEY_7,Pin_Key_7)
#define KEY_0        GPIO_ReadInputDataBit(GPIO_KEY_8,Pin_Key_8)
#define KEY_OK       GPIO_ReadInputDataBit(GPIO_KEY_9,Pin_Key_9)
#define KEY_DN       GPIO_ReadInputDataBit(GPIO_KEY_UP,Pin_Key_UP)
#define KEY_2        GPIO_ReadInputDataBit(GPIO_KEY_DOWN,Pin_Key_DOWN)
#define KEY_LF       GPIO_ReadInputDataBit(GPIO_KEY_LF,Pin_Key_LF)
#define KEY_RT       GPIO_ReadInputDataBit(GPIO_KEY_RT,Pin_Key_RT)
#define KEY_MENU     GPIO_ReadInputDataBit(GPIO_KEY_MENU,Pin_Key_MENU)
#define KEY_INPUT    GPIO_ReadInputDataBit(GPIO_KEY_DEL,Pin_Key_DEL)
#define KEY_3        GPIO_ReadInputDataBit(GPIO_KEY_INPUT,Pin_Key_INPUT)
#define KEY_UP       GPIO_ReadInputDataBit(GPIO_KEY_MODE,Pin_Key_MODE)
#define KEY_DEL      GPIO_ReadInputDataBit(GPIO_KEY_OK,Pin_Key_OK)

/*重新对按键进行定义*/

typedef enum{
	Open_GUI,        //开机界面
	Main_GUI,        //主界面	
	Config_GUI,      //配置界面
	Service_GUI,     //维修界面	
	Test_GUI,        //测试界面
	Close_GUI,       //关机界面
	
	GateWay_Set,     //配置模式下，网关选择
	Address_Set,     //配置模式下，设置ZigBee地址
	Config_Set,      //配置模式下，高级配置，可配置所有参数
	Config_DIS,      //配置模式下，配置显示
	
	GateWay_Choose,  //维修模式下，网关选择
	Address_Choose,  //维修模式下，地址选择
	Read_Data,       //维修模式下，读取镇流器参数
	Ballast_Operate, //维修模式下，镇流器操作  /*包括开灯、关灯*/
	Diagn_Reason,    //维修原因    /*包括镇流器问题、灯管问题、灯座问题、ZigBee模块问题、电源问题、接触不良问题、ZigBee地址问题*/
	
	GateWay_Decide,  //测试模式下，网关选择
	Address_Option,  //测试模式下，ZigBee地址选择
	Debug_Option,    //测试模式下，调试镇流器 /*包括开灯、关灯、调光，读镇流器数据*/
	
}Dis_Type;


void key_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 

	GPIO_InitStructure.GPIO_Pin = Pin_Key_0;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(GPIO_KEY_0, &GPIO_InitStructure);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//PB3用作普通IO
	
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

/*
static void Key_Handle(void){
	Dis_Type Status;

	switch(Status){
		case Open_GUI: 
			
			break;
		case Main_GUI:
			
			break;
		case Config_GUI:
			
			break;
		case Service_GUI:
			break;
		case Test_GUI:
			break;
		case Close_GUI:
			break;
		case GateWay_Set:
			break;
		case Address_Set
		
	}
	
}
*/



