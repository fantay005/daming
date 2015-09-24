#include <string.h>
#include <stdio.h>
#include "display.h"
#include "ili9320_api.h"
#include "ili9320.h"
#include "key.h"
#include "CommZigBee.h"

#define BACKCOLOR   LIGHTBLUE
#define LINECOLOR   BLACK
#define DAKENCOLOR  DARKBLUE

#define XStartFont  4
#define XStartParam 160

#define YStartLine  10

#define YFirstApart  4
#define YLineApart   18

ZigBee_Param Config_ZigBee, Display_ZigBee; 

BSN_Data Analysis_MSG;


static void BackColorSet(void){
	ili9320_Clear(BACKCOLOR);
}

static void DarkenPrepareLine(char line){
	ili9320_Darken(line, DAKENCOLOR);
}

static void Boot_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(60, 116, "精益求精");
	Lcd_DisplayChinese32(100, 116, "追求卓越");
	
	Lcd_DisplayChinese16(192, 80, "合肥大明节能科技股份有限公司");
	Lcd_DisplayChinese16(216, 80, "www.fitbright.cn");
}

static unsigned short YCoordValue(unsigned char line){
	return (43 + (line - 1) * 18);
}

static void Main_Interface(void){
  unsigned short i = 1;	

	BackColorSet();
	
	Lcd_DisplayChinese32(112, YFirstApart, "主选项");
	GUI_Line(0,40,319,40,LINECOLOR);
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.配置");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.维修");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.调试");
}

static void Config_Interface(void){
	unsigned short i = 1;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "配置选项");
	GUI_Line(0,40,319,40,LINECOLOR);
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.网关选择");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.ZigBee地址");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.高级设置");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "4.显示配置");
	i++;
}

static void Repair_Interface(void){
	unsigned short i;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "维修选项");
	GUI_Line(0,40,319,40,LINECOLOR);
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.网关选项");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.ZigBee地址");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.读镇流器数据");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "4.开/关灯");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "5.原因分析");
}

static void Debug_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "调试选项");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
}

static void Gateway_Option_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "网关选择");
	GUI_Line(0,40,319,40,LINECOLOR);	
}

static void Addr_Option_Interface(void){
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "ZigBee地址配置");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
}

static void Ballast_Data_Interface(void){
	unsigned short i = 1;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "读镇流器数据");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "工作状态：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.STATE);
	i++;
	
  Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "调 光 值：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.DIM);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "输入电压：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.INPUT_VOL);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "输入电流：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.INPUT_CUR);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "输入功率：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.INPUT_POW);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "灯 电 压：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.LIGHT_VOL);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "PFC 电压：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.PFC_VOL);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "温    度：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.TEMP);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "运行时间：");
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), Analysis_MSG.TIME);
	i++;
}

static void Reason_Analyze_Interface(void){
	unsigned short i;
	BackColorSet();
	
	Lcd_DisplayChinese32(100, YFirstApart, "原因分析");
	GUI_Line(0,40,319,40,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "1.镇流器");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "2.灯管");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "3.灯座");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "4.电源");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "5.ZigBee模块");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "6.接口接触不良");
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordValue(i), "7.其它");
	i++;
}

static unsigned short YCoordDot(unsigned char line){
	return (YStartLine + YFirstApart + line * YLineApart);
}

static void Advanced_Config_Interface(ZigBee_Param Config_ZigBee){
	unsigned char i = 0;
	
	BackColorSet();
	
	GUI_Line(0,YStartLine,319,YStartLine,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 1.节点地址：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.MAC_ADDR);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 2.节点名称：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NODE_NAME);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 3.节点类型：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NODE_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 4.网络类型：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NET_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 5.网 络 ID：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.NET_ID);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 6.无线频点：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.FREQUENCY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 7.地址编码：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.DATA_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 8.发送模式：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.TX_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 9.波 特 率：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.BAUDRATE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "10.校    验：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.DATA_PARITY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "11.数 据 位：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.DATA_BIT);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "12.数据源址：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Config_ZigBee.SRC_ADR);
	i++;
	
	GUI_Line(0, YCoordDot(i), 319, YCoordDot(i), LINECOLOR);
	
}

static void Display_Config_Interface(ZigBee_Param Display_ZigBee){
	unsigned char i = 0;
	
	BackColorSet();
	
	GUI_Line(0,10,319,10,LINECOLOR);	
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 1.节点地址：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.MAC_ADDR);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 2.节点名称：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NODE_NAME);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 3.节点类型：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NODE_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 4.网络类型：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NET_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i),  " 5.网 络 ID：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.NET_ID);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 6.无线频点：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.FREQUENCY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 7.地址编码：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.DATA_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 8.发送模式：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.TX_TYPE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), " 9.波 特 率：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.BAUDRATE);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "10.校    验：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.DATA_PARITY);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "11.数 据 位：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.DATA_BIT);
	i++;
	
	Lcd_DisplayChinese16(XStartFont, YCoordDot(i), "12.数据源址：");
	Lcd_DisplayChinese16(XStartParam, YCoordDot(i), Display_ZigBee.SRC_ADR);
	i++;
	
	GUI_Line(0, YCoordDot(i), 319, YCoordDot(i), LINECOLOR);
}

void ZigBeeParamInit(ZigBee_Param msg){
	sprintf(msg.BAUDRATE, "9600");
	sprintf(msg.DATA_BIT, "8+0+1");
	sprintf(msg.DATA_PARITY, "None");
	sprintf(msg.DATA_TYPE, "HEX输出");
	sprintf(msg.FREQUENCY, "0F");
	sprintf(msg.MAC_ADDR, "0001");
	sprintf(msg.NET_ID, "FF");
	sprintf(msg.NET_TYPE, "星型网");
	sprintf(msg.NODE_NAME, "SHUNCOM");
	sprintf(msg.NODE_TYPE, "中继路由");
	sprintf(msg.SRC_ADR, "不输出");
	sprintf(msg.TX_TYPE, "广播");
}

void DisplayInit(void){
	ZigBeeParamInit(Config_ZigBee);
	ZigBeeParamInit(Display_ZigBee);
	
	
}