#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x_gpio.h"

#define  KEY_IO_0     GPIOB     
#define  KEY_Pin_0    GPIO_Pin_8 

#define  KEY_IO_1     GPIOB     
#define  KEY_Pin_1    GPIO_Pin_0 

#define  KEY_IO_2     GPIOC     
#define  KEY_Pin_2    GPIO_Pin_1  

#define  KEY_IO_3     GPIOC   
#define  KEY_Pin_3    GPIO_Pin_10 

#define  KEY_IO_4     GPIOB    
#define  KEY_Pin_4    GPIO_Pin_1

#define  KEY_IO_5     GPIOB   
#define  KEY_Pin_5    GPIO_Pin_2  

#define  KEY_IO_6     GPIOB  
#define  KEY_Pin_6    GPIO_Pin_3

#define  KEY_IO_7     GPIOB     
#define  KEY_Pin_7    GPIO_Pin_4 

#define  KEY_IO_8     GPIOB   
#define  KEY_Pin_8    GPIO_Pin_5   

#define  KEY_IO_9     GPIOB   
#define  KEY_Pin_9    GPIO_Pin_6 

#define  KEY_IO_Lf    GPIOC  
#define  KEY_Pin_Lf   GPIO_Pin_2 

#define  KEY_IO_Rt    GPIOC   
#define  KEY_Pin_Rt   GPIO_Pin_3 

#define  KEY_IO_Up    GPIOA  
#define  KEY_Pin_Up   GPIO_Pin_5 

#define  KEY_IO_Dn    GPIOC  
#define  KEY_Pin_Dn   GPIO_Pin_0 

typedef enum{
	KEY0,
	KEY1,
	KEY2,
	KEY3,
	KEY4,
	KEY5,
	KEY6,
	KEY7,
	KEY8,
	KEY9,
	KEYA,
	KEYB,
	KEYC,
	KEYD,
	KEYE,
	KEYF,
	KEYL,
	KEYUP,
	KEYDN,
	KEYLF,
	KEYRT,
	KEYOK,
	KEYMENU,
	KEYINPT,
	KEYDEL,
	KEYCLEAR,
	NOKEY,
}KeyPress;

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


void key_init(void);
void KEY(void);

#endif /* __KEY_H */
