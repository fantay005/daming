#include <string.h>
#include <stdio.h>
#include "display.h"
#include "ili9320_api.h"
#include "ili9320.h"
#include "key.h"


void Boot_Interface(void){
	
	Lcd_DisplayChinese32(60, 116, (const unsigned char *)"精益求精");
	Lcd_DisplayChinese32(100, 116, (const unsigned char *)"追求卓越");
	
	Lcd_DisplayChinese16(192, 80, (const unsigned char *)"合肥大明节能科技股份有限公司");
	Lcd_DisplayChinese16(216, 80, (const unsigned char *)"www.fitbright.cn");
}

void Main_Interface(void){
	Lcd_DisplayChinese32(4, 112, "主选项");
	GUI_Line(40,16,319,16,BLACK);
	
}
