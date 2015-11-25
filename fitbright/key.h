#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x_gpio.h"
#include "stdbool.h"

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
	NOKEY = 0,
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
	KEYF = 16,
	KEYL,
	KEYUP,
	KEYDN,
	KEYLF = 20,
	KEYRT,
	KEYOK,
	KEYMENU = 150,
	KEYINPT,
	KEYCONF,
	KEYRETURN,
}KeyPress;

typedef enum{
	Open_GUI = 1,    		//开机界面
	Main_GUI,        		//主界面	
	Project_Dir,     		//项目目录
	Config_GUI,      		//配置界面
	Service_GUI,     		//维修界面	
	Test_GUI,        		//测试界面
	Intro_GUI,       		//简介界面
	
	GateWay_Set = 8, 		//配置模式下，网关选择
	Address_Set,     		//配置模式下，设置ZigBee地址
	Config_Set,      		//配置模式下，高级配置，可配置所有参数
	Config_DIS,      		//配置模式下，配置显示
	
	GateWay_Choose = 12,//维修模式下，网关选择
	Address_Choose,  		//维修模式下，地址选择
	Read_Data,       		//维修模式下，读取镇流器参数
//	Ballast_Operate, 	//维修模式下，镇流器操作  /*包括开灯、关灯*/
//	Diagn_Reason,    	//维修原因    /*包括镇流器问题、灯管问题、灯座问题、ZigBee模块问题、电源问题、接触不良问题、ZigBee地址问题*/
	
	GateWay_Decide = 15,//测试模式下，网关选择
	Address_Option,  		//测试模式下，ZigBee地址选择
	Real_MOnitor,    		//测试模式下，实时监控
	Light_Dim,       		//测试模式下，调光
	Light_Attrib,      	//测试模式下，灯属性选择
	
	Frequ_Set = 20,     //配置模式下，频点选择
	Frequ_Choose,    		//维修模式下，频点选择
	Frequ_Option,    		//测试模式下，频点选择
	
	Lamp_Pole = 23,     //维修模式下，灯杆操作
	Map_Dis,            //维修模式下，显示地址所在点
	Listen_Into,        //测试模式下，监听板控制界面
}Dis_Type;

typedef enum{
	Pro_Null,
	Pro_BinHu,
	Pro_ChanYeYuan,
	Pro_DaMing,
}pro;

extern pro Project;                 //初始化项目为无
extern unsigned char GateWayID;     //网关序列号
extern unsigned char FrequencyDot;  //初始频点为无
extern unsigned int  ZigBAddr ;     //初始ZigBee地址值
extern char Config_Enable;          //配置键使能配置模块功能
extern char Digits;                 //点亮、修改地址的位数
extern char MaxBit;                 //最大位数
extern char BaseBit;                //初始数字的 起始位
extern bool HexSwitchDec;           //十六进制与十进制切换
extern char StartRead;              //开始读取镇流器数据
extern Dis_Type  InterFace;         //当前显示界面

void key_init(void);
void KEY(void);

#endif /* __KEY_H */
