#include "display.h"
#include "ili9320_api.h"
#include "ili9320.h"
#include "key.h"
#include <string.h>

void ZigbeeConfigDisplay(void)
{
	
	ili9320_Clear(GREEN);
	
    /************************×ó²àÀ¸*****************************/
	GUI_Text(30,0,(u8 *)"ZIGBEE Information",MAGENTA,GREEN);		 
	
	GUI_Text(10,20,(u8 *)"MAC_Addr :",BLACK,GREEN);			

	GUI_Text(10,36,(u8 *)"Net_ID   :",BLACK,GREEN);

	GUI_Text(10,52,(u8 *)"Channel  :",BLACK,GREEN);

  GUI_Text(10,68,(u8 *)"Node_Type:",BLACK,GREEN);

	GUI_Text(10,84,(u8 *)"Node_Name:",BLACK,GREEN);
		
	GUI_Text(10,100,(u8 *)"Net_Type :",BLACK,GREEN);

	GUI_Text(10,116,(u8 *)"Data_Type:",BLACK,GREEN);

	GUI_Text(10,132,(u8 *)"Tx_Type  :",BLACK,GREEN);

	GUI_Text(10,148,(u8 *)"Baud_Rate:",BLACK,GREEN);

	GUI_Text(10,164,(u8 *)"Parity   :",BLACK,GREEN);

	GUI_Text(10,180,(u8 *)"Data_Bit :",BLACK,GREEN);

	GUI_Text(10,196,(u8 *)"Src_Addr :",BLACK,GREEN);
	

	/************************·Ö¸ô·û*****************************/
//	GUI_Line(0,16,319,16,BLACK);
//	GUI_Line(0,214,319,214,BLACK);
//	GUI_Line(200,0,200,214,BLACK);
	/************************ÓÒ²àÀ¸*****************************/
	
	while(1);

}

