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
	Open_GUI = 1,    		//��������
	Main_GUI,        		//������	
	Project_Dir,     		//��ĿĿ¼
	Config_GUI,      		//���ý���
	Service_GUI,     		//ά�޽���	
	Test_GUI,        		//���Խ���
	Intro_GUI,       		//������
	
	GateWay_Set = 8, 		//����ģʽ�£�����ѡ��
	Address_Set,     		//����ģʽ�£�����ZigBee��ַ
	Config_Set,      		//����ģʽ�£��߼����ã����������в���
	Config_DIS,      		//����ģʽ�£�������ʾ
	
	GateWay_Choose = 12,//ά��ģʽ�£�����ѡ��
	Address_Choose,  		//ά��ģʽ�£���ַѡ��
	Read_Data,       		//ά��ģʽ�£���ȡ����������
//	Ballast_Operate, 	//ά��ģʽ�£�����������  /*�������ơ��ص�*/
//	Diagn_Reason,    	//ά��ԭ��    /*�������������⡢�ƹ����⡢�������⡢ZigBeeģ�����⡢��Դ���⡢�Ӵ��������⡢ZigBee��ַ����*/
	
	GateWay_Decide = 15,//����ģʽ�£�����ѡ��
	Address_Option,  		//����ģʽ�£�ZigBee��ַѡ��
	Real_MOnitor,    		//����ģʽ�£�ʵʱ���
	Light_Dim,       		//����ģʽ�£�����
	Light_Attrib,      	//����ģʽ�£�������ѡ��
	
	Frequ_Set = 20,     //����ģʽ�£�Ƶ��ѡ��
	Frequ_Choose,    		//ά��ģʽ�£�Ƶ��ѡ��
	Frequ_Option,    		//����ģʽ�£�Ƶ��ѡ��
	
	Lamp_Pole = 23,     //ά��ģʽ�£��Ƹ˲���
	Map_Dis,            //ά��ģʽ�£���ʾ��ַ���ڵ�
	Listen_Into,        //����ģʽ�£���������ƽ���
}Dis_Type;

typedef enum{
	Pro_Null,
	Pro_BinHu,         /*������Ŀ*/
	Pro_ChanYeYuan,    /*��ɽ��ҵ԰��Ŀ*/
	Pro_DaMing,        /*������˾��Ŀ*/
	Pro_YaoHai,        /*������Ŀ*/
	Pro_JingKai,       /*������Ŀ*/
	Pro_XinZhan,       /*��վ��Ŀ*/
}pro;

extern pro Project;                 //��ʼ����ĿΪ��
extern unsigned char GateWayID;     //�������к�
extern unsigned char FrequencyDot;  //��ʼƵ��Ϊ��
extern unsigned int  ZigBAddr ;     //��ʼZigBee��ֵַ
extern char Config_Enable;          //���ü�ʹ������ģ�鹦��
extern char Digits;                 //�������޸ĵ�ַ��λ��
extern char MaxBit;                 //���λ��
extern char BaseBit;                //��ʼ���ֵ� ��ʼλ
extern bool HexSwitchDec;           //ʮ��������ʮ�����л�
extern char StartRead;              //��ʼ��ȡ����������
extern Dis_Type  InterFace;         //��ǰ��ʾ����

void key_init(void);
void KEY(void);

#endif /* __KEY_H */
