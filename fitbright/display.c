#include "display.h"
#include "ili9320_api.h"
#include "ili9320.h"
#include "key.h"
#include <string.h>

void ZigbeeConfigDisplay(void)
{
	
	ili9320_Clear(GREEN);
	
    /************************×ó²àÀ¸*****************************/
	
	GUI_Text(60,0,(u8 *)"Function Select",MAGENTA,GREEN);		
	
	GUI_Line(0,20,319,20,BLACK);
	
	GUI_Text(10,24,(u8 *)"1: Zigbee Configuration",BLACK,GREEN);	
	
	GUI_Text(10,44,(u8 *)"2: Gateway Option",BLACK,GREEN);
	
	GUI_Text(10,64,(u8 *)"3: Communication Test",BLACK,GREEN);
	
	GUI_Text(10,84,(u8 *)"4: Zigbee Information",BLACK,GREEN);
	

	/************************·Ö¸ô·û*****************************/
//	GUI_Line(0,16,319,16,BLACK);
//	GUI_Line(0,214,319,214,BLACK);
//	GUI_Line(200,0,200,214,BLACK);
	/************************ÓÒ²àÀ¸*****************************/
	
	while(1);

}
