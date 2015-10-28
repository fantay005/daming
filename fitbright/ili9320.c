/**
  ******************************************************************************
  * @file    stm3210e_eval_lcd.c
  * @author  ARMJISHU Application Team
  * @version V4.5.0
  * @date    07-March-2011
  * @brief   This file includes the LCD driver for AM-240320L8TNQW00H 
  *          (LCD_ILI9320) and AM-240320LDTNQW00H (LCD_SPFD5408B) Liquid Crystal
  *          Display Module of STM3210E-EVAL board.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_fsmc.h"
#include "ili9320.h"
#include "ili9320_font.h"
#include "stm32f10x_gpio.h"
#include <stdio.h>
#include "font_dot_array.h"
#include "zklib.h"
#include "sdcard.h"
#include "key.h"

#define ILI_TASK_STACK_SIZE			 (configMINIMAL_STACK_SIZE + 1024)

static xQueueHandle __Ili9320Queue;

#define FontColor     BLACK

#define BackColor     CYAN
#define EnLight       WHITE
#define TestColor     YELLOW

#define NodeColor     BLUE            //中心节点信息颜色
#define DealColor     MAGENTA         //待处理提示信息颜色
#define InPutColor    RED             //输入法颜色
#define ParamColor    BRRED           //显示灯参颜色

#define LED_DOT_WIDTH 320


static unsigned char arrayBuffer[128];

u16 DeviceCode;

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  vu16 LCD_REG;
  vu16 LCD_RAM;
} LCD_TypeDef;

/* LCD is connected to the FSMC_Bank1_NOR/SRAM4 and NE4 is used as ship select signal */
#define LCD_BASE   ((u32)(0x60000000 | 0x0C000000))
#define LCD         ((LCD_TypeDef *) LCD_BASE)

//#define SetCs  
//#define ClrCs  

#define SetCs  GPIO_SetBits(GPIOG, GPIO_Pin_12);
#define ClrCs  GPIO_ResetBits(GPIOG, GPIO_Pin_12);

//#define SetCs  GPIO_SetBits(GPIOB, GPIO_Pin_8);
//#define ClrCs  GPIO_ResetBits(GPIOB, GPIO_Pin_8);

////////////////////////////////////////////////////////////////////////////////////////
/*******************************************************************************
* Function Name  : LCD_CtrlLinesConfig
* Description    : Configures LCD Control lines (FSMC Pins) in alternate function
                   Push-Pull mode.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_CtrlLinesConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable FSMC, GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks */

  /* Set PD.00(D2), PD.01(D3), PD.04(NOE), PD.05(NWE), PD.08(D13), PD.09(D14),
     PD.10(D15), PD.14(D0), PD.15(D1) as alternate 
     function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 |
                                GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Set PE.07(D4), PE.08(D5), PE.09(D6), PE.10(D7), PE.11(D8), PE.12(D9), PE.13(D10),
     PE.14(D11), PE.15(D12) as alternate function push pull */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* Set PC15(A0 (RS)) as alternate function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  /* Set PG.12(NE4 (LCD/CS)) as alternate function push pull - CE3(LCD /CS) */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOG, &GPIO_InitStructure);

  /*FSMC A21和A22初试化，推挽复用输出*/ 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6; 
  GPIO_Init(GPIOE, &GPIO_InitStructure); 

  /* Lcd_Light_Control */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  //Lcd_Light_OFF  
  Lcd_Light_ON

  /* LEDs pins configuration */
#if 0
  /* Set PD7 to disable NAND */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOD, GPIO_Pin_7);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOG, GPIO_Pin_9 | GPIO_Pin_10);

  /* Set PC7 to disable NAND */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOC, GPIO_Pin_7);
#endif
}

/*******************************************************************************
* Function Name  : LCD_FSMCConfig
* Description    : Configures the Parallel interface (FSMC) for LCD(Parallel mode)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_FSMCConfig(void)
{
  FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  Timing_read, Timing_write;

/*-- FSMC Configuration ------------------------------------------------------*/
/*----------------------- SRAM Bank 4 ----------------------------------------*/
  /* FSMC_Bank1_NORSRAM4 configuration */
  Timing_read.FSMC_AddressSetupTime = 6;             
  Timing_read.FSMC_AddressHoldTime = 0;  
  Timing_read.FSMC_DataSetupTime = 6; 
  Timing_read.FSMC_BusTurnAroundDuration = 0;
  Timing_read.FSMC_CLKDivision = 0;
  Timing_read.FSMC_DataLatency = 0;
  Timing_read.FSMC_AccessMode = FSMC_AccessMode_A;    

  Timing_write.FSMC_AddressSetupTime = 2;             
  Timing_write.FSMC_AddressHoldTime = 0;  
  Timing_write.FSMC_DataSetupTime = 2; 
  Timing_write.FSMC_BusTurnAroundDuration = 0;
  Timing_write.FSMC_CLKDivision = 0;
  Timing_write.FSMC_DataLatency = 0;  
  Timing_write.FSMC_AccessMode = FSMC_AccessMode_A; 
   
  /* Color LCD configuration ------------------------------------
     LCD configured as follow:
        - Data/Address MUX = Disable
        - Memory Type = SRAM
        - Data Width = 16bit
        - Write Operation = Enable
        - Extended Mode = Enable
        - Asynchronous Wait = Disable */
  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &Timing_read;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &Timing_write;

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  

  /* BANK 4 (of NOR/SRAM Bank 1~4) is enabled */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
     
}


void LCD_X_Init(void)
{
 /* Configure the LCD Control pins --------------------------------------------*/
  LCD_CtrlLinesConfig();

/* Configure the FSMC Parallel interface -------------------------------------*/
  LCD_FSMCConfig();
}


/*******************************************************************************
* Function Name  : LCD_WriteReg
* Description    : Writes to the selected LCD register.
* Input          : - LCD_Reg: address of the selected register.
*                  - LCD_RegValue: value to write to the selected register.
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteReg(u16 LCD_Reg,u16 LCD_RegValue)
{
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  LCD->LCD_REG = LCD_Reg;
  /* Write 16-bit Reg */
  LCD->LCD_RAM = LCD_RegValue;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_ReadReg
* Description    : Reads the selected LCD Register.
* Input          : None
* Output         : None
* Return         : LCD Register Value.
*******************************************************************************/
u16 LCD_ReadReg(u8 LCD_Reg)
{
  u16 data;
  /* Write 16-bit Index (then Read Reg) */
  ClrCs
  //LCD->LCD_REG = LCD_Reg;
  data = LCD->LCD_RAM; 
    SetCs
     return    data;

  /* Read 16-bit Reg */
  //return (LCD->LCD_RAM);
}

u16 LCD_ReadSta(void)
{
  u16 data;
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  data = LCD->LCD_REG;
  SetCs
  return    data;
}

void LCD_WriteCommand(u16 LCD_RegValue)
{
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  LCD->LCD_REG = LCD_RegValue;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_WriteRAM_Prepare
* Description    : Prepare to write to the LCD RAM.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteRAM_Prepare(void)
{
  ClrCs
  LCD->LCD_REG = R34;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_WriteRAM
* Description    : Writes to the LCD RAM.
* Input          : - RGB_Code: the pixel color in RGB mode (5-6-5).
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteRAM(u16 RGB_Code)					 
{
  ClrCs
  /* Write 16-bit GRAM Reg */
  LCD->LCD_RAM = RGB_Code;
  SetCs
}

/*******************************************************************************
* Function Name  : LCD_ReadRAM
* Description    : Reads the LCD RAM.
* Input          : None
* Output         : None
* Return         : LCD RAM Value.
*******************************************************************************/
u16 LCD_ReadRAM(void)
{
  u16 dummy;
  u16 data;
  /* Write 16-bit Index (then Read Reg) */
  ClrCs
  LCD->LCD_REG = R34; /* Select GRAM Reg */
  /* Read 16-bit Reg */
  dummy = LCD->LCD_RAM; 
  dummy++;
  data = LCD->LCD_RAM; 
  SetCs
  return    data;
  //return LCD->LCD_RAM;
}

/*******************************************************************************
* Function Name  : LCD_SetCursor
* Description    : Sets the cursor position.
* Input          : - Xpos: specifies the X position.
*                  - Ypos: specifies the Y position. 
* Output         : None
* Return         : None
*******************************************************************************/


void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
  LCD_WriteReg(0x06,Ypos>>8);
  LCD_WriteReg(0x07,Ypos);
  
  LCD_WriteReg(0x02,Xpos>>8);
  LCD_WriteReg(0x03,Xpos);  
}			 

void Delay_nms(int n)
{
  
  u32 f=n,k;
  for (; f!=0; f--)
  {
    for(k=0xFFF; k!=0; k--);
  }
  
}

void Delay(u32 nCount)
{
 u32 TimingDelay; 
 while(nCount--)
   {
    for(TimingDelay=0;TimingDelay<10000;TimingDelay++);
   }
}

/****************************************************************************
* 名    称：void ili9320_SetCursor(u16 x,u16 y)
* 功    能：设置屏幕座标
* 入口参数：x      行座标
*           y      列座标
* 出口参数：无
* 说    明：
* 调用方法：ili9320_SetCursor(10,10);
****************************************************************************/
//inline void ili9320_SetCursor(u16 x,u16 y)
void ili9320_SetCursor(u16 x,u16 y)
{
	if(DeviceCode==0x8989)
	{
	 	LCD_WriteReg(0x004e,y);        //行
    	LCD_WriteReg(0x004f,x);  //列
	}
	else if((DeviceCode==0x9919))
	{
		LCD_WriteReg(0x004e,x); // 行
  		LCD_WriteReg(0x004f,y); // 列	
	}
    /*
	else if((DeviceCode==0x9325))
	{
		LCD_WriteReg(0x0020,x); // 行
  		LCD_WriteReg(0x0021,y); // 列	
	}
	*/
	else
	{
  		LCD_WriteReg(0x0020,y); // 行
  		LCD_WriteReg(0x0021,0x13f-x); // 列
	}
}

/**
  * @brief  Draws a chinacharacter on LCD.
  * @param  Xpos: the Line where to display the character shape.
  * @param  Ypos: start column address.
  * @param  c: pointer to the character data.
  * @retval None
  */

void LCD_DrawChinaChar24x24(u8 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 24; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(j = 0; j < 3; j++)
    {
        for(i = 0; i < 8; i++)
        {
            if((c[3*index + j] & (0x80 >> i)) == 0x00)
                LCD_WriteRAM(bkColor);
            else
                LCD_WriteRAM(charColor);
        }   
    }
    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}


void LCD_DrawChinaChar16x16(u16 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 16; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(j = 0; j < 2; j++)
    {
        for(i = 0; i < 8; i++)
        {
            if((c[2*index + j] & (0x80 >> i)) == 0x00)
                LCD_WriteRAM(bkColor);
            else
                LCD_WriteRAM(charColor);
        }   
    }
    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}

void LCD_DrawChinaChar16x8(u16 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 16; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */

		for(i = 0; i < 8; i++)
		{
			if((c[index] & (0x80 >> i)) == 0x00)
					LCD_WriteRAM(bkColor);
			else
					LCD_WriteRAM(charColor);
		}

    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}

void LCD_DrawChinaChar32x16(u16 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 32; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */

		for(j = 0; j < 2; j++){
			for(i = 0; i < 8; i++)
			{
					if((c[2*index + j] & (0x80 >> i)) == 0x00)
							LCD_WriteRAM(bkColor);
					else
							LCD_WriteRAM(charColor);
			}
		}

    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}

void LCD_DrawChinaChar32x32(u16 Xpos, u16 Ypos, const u8 *c,u16 charColor,u16 bkColor)
{
  u32 index = 0, i = 0, j = 0;
  u8 Xaddress = 0;
   
  Xaddress = Xpos;

  ili9320_SetCursor(Ypos,Xaddress);

  for(index = 0; index < 32; index++)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(j = 0; j < 4; j++)
    {
				for(i = 0; i < 8; i++)
        {
            if((c[4*index + j] & (0x80 >> i)) == 0x00)
                LCD_WriteRAM(bkColor);
            else
                LCD_WriteRAM(charColor);
        }  				
    }
    Xaddress++;
    ili9320_SetCursor(Ypos, Xaddress);
  }
}

const unsigned char *LedDisplayGB2312String(int y, int x, const unsigned char *gbString, unsigned char  DotMatrix, int charColor, int bkColor) {
	if(gbString == NULL)
	return NULL;
		
	
	if (!FontDotArrayFetchLock()) {
		return gbString;
	}

	while (*gbString) {
		if (isAsciiStart(*gbString)) {
			if(*gbString == 0x0D){
					++gbString;
					continue;
			}
			if(*gbString == 0x0A){
				if(*(gbString - 1) == 0x0D){
					x += BYTES_HEIGHT_PER_FONT_ASCII_16X8;
					++gbString;
					y = 0;
					if(x > 230){
						goto __exit;
					}
					continue;
				}
			}
			
			if(x > 230){
				goto __exit;
			}

			if(DotMatrix == 16){
				if (y > LED_DOT_WIDTH - BYTES_WIDTH_PER_FONT_ASCII_16X8) {
					x += BYTES_HEIGHT_PER_FONT_ASCII_16X8;
					y = 0;
					if(x > 230){
						goto __exit;
					}
				}
				FontDotArrayFetchASCII_16(arrayBuffer, *gbString);
				LCD_DrawChinaChar16x8(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				y += BYTES_WIDTH_PER_FONT_ASCII_16X8;
			} else if(DotMatrix == 32){
				if (y > LED_DOT_WIDTH - BYTES_WIDTH_PER_FONT_ASCII_32X16) {
					x += BYTES_HEIGHT_PER_FONT_ASCII_32X16;
					y = 0;
					if(x > 230){
						goto __exit;
					}
				}
				FontDotArrayFetchASCII_32(arrayBuffer, *gbString);
				LCD_DrawChinaChar32x16(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				y += BYTES_WIDTH_PER_FONT_ASCII_32X16;
			}
				++gbString;

		} else if (isGB2312Start(*gbString)) {
			int code = (*gbString++) << 8;
			if (!isGB2312Start(*gbString)) {
				goto __exit;
			}
			code += *gbString++;
			if(DotMatrix == 16){
				if (y > LED_DOT_WIDTH - BYTES_HEIGHT_PER_FONT_GB_16X16) {
					x += BYTES_HEIGHT_PER_FONT_GB_16X16;
					y = 0;
					if(x > 230){
						goto __exit;
					}
				}
				FontDotArrayFetchGB_16(arrayBuffer, code);
				LCD_DrawChinaChar16x16(x, y, (const u8 *)arrayBuffer, charColor, bkColor);		
				y += BYTES_WIDTH_PER_FONT_GB_16X16;
			} else if (DotMatrix == 32){
				if (y > LED_DOT_WIDTH - BYTES_WIDTH_PER_FONT_GB_32X32) {
					x += BYTES_WIDTH_PER_FONT_GB_32X32;
					y = 0;
					if(x > 230){
						goto __exit;
					}
				}
				FontDotArrayFetchGB_32(arrayBuffer, code);
				LCD_DrawChinaChar32x32(x, y, (const u8 *)arrayBuffer, charColor, bkColor);		
				y += BYTES_WIDTH_PER_FONT_GB_32X32;
			}
				
		} else if (isUnicodeStart(*gbString)) {
			int code = (*gbString++) << 8;
			code += *gbString++;
		
			if(DotMatrix == 16){
				FontDotArrayFetchUCS_16(arrayBuffer, code);
				LCD_DrawChinaChar16x16(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				y += BYTES_WIDTH_PER_FONT_GB_16X16;
			} else if (DotMatrix == 32){
				FontDotArrayFetchUCS_32(arrayBuffer, code);
				LCD_DrawChinaChar32x32(x, y, (const u8 *)arrayBuffer, charColor, bkColor);
				y += BYTES_WIDTH_PER_FONT_GB_32X32;
			}
			
		} else {
			++gbString;
		}
	}
__exit:
	FontDotArrayFetchUnlock();
	if (*gbString) {
		return gbString;
	}
	return NULL;
}


const unsigned char *Lcd_LineDisplay16(char line, const unsigned char *str){	
	return LedDisplayGB2312String(0, (line - 1)*16, str, 16, FontColor, BackColor);
}

const unsigned char *Lcd_DisplayInput(int x, int y, const unsigned char *str){
	
	return LedDisplayGB2312String(x, y, str, 16, InPutColor, BackColor);
}

const unsigned char *Lcd_DisplayLightParam(int x, int y, const unsigned char *str){
	
	return LedDisplayGB2312String(x, y, str, 16, ParamColor, BackColor);
}

const unsigned char *Lcd_DisplayDeal(int x, int y, const unsigned char *str){
	
	return LedDisplayGB2312String(x, y, str, 16, DealColor, BackColor);
}

const unsigned char *Lcd_DisplayNode(int x, int y, const unsigned char *str){
	
	return LedDisplayGB2312String(x, y, str, 16, NodeColor, BackColor);
}

const unsigned char *Lcd_DisplayChinese16(int x, int y, const unsigned char *str){
	
	return LedDisplayGB2312String(x, y, str, 16, FontColor, BackColor);
}

const unsigned char *Lcd_DisplayChinese32(int x, int y, const unsigned char *str){
	
	return LedDisplayGB2312String(x, y, str, 32, FontColor, BackColor);
}

/****************************************************************************
* 名    称：void ili9320_Clear(u16 dat)
* 功    能：将屏幕填充成指定的颜色，如清屏，则填充 0xffff
* 入口参数：dat      填充值
* 出口参数：无
* 说    明：
* 调用方法：ili9320_Clear(0xffff);
****************************************************************************/
void ili9320_Clear(u16 Color)
{
  u32 index=0;
  ili9320_SetCursor(0,0); 
  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(index=0;index<76800;index++)
   {
     LCD->LCD_RAM=Color;
   }
}

/****************************************************************************
清除一行数据

*****************************************************************************/

void TFT_ClearLine(u16 row)
{
  u32 index = (row - 1) * 320;
	u32 MaxIndex = index + 320 * 16 - 1;
  ili9320_SetCursor(index,0); 
  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(;index<MaxIndex;index++)
   {
     LCD->LCD_RAM=BackColor;
   }
}

/****************************************************************************
* 名    称：void ili9320_Initializtion()
* 功    能：初始化 ILI9320 控制器
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：ili9320_Initializtion();
****************************************************************************/
void ili9320_Initializtion(void)
{
    /*****************************
    **    硬件连接说明          **
    ** STM32         ili9320    **
    ** PE0~15 <----> DB0~15     **
    ** PD15   <----> nRD        **
    ** PD14   <----> RS         **
    ** PD13   <----> nWR        **
    ** PD12   <----> nCS        **
    ** PD11   <----> nReset     **
    ** PC0    <----> BK_LED     **
    ******************************/

    LCD_X_Init();
    Delay(5); /* delay 50 ms */
    LCD_WriteReg(0x0000,0x0001);  
    Delay(5); /* delay 50 ms */			//start internal osc
    DeviceCode = LCD_ReadReg(0x0000);
    Delay(5); /* delay 50 ms */
    DeviceCode = LCD_ReadReg(0x0000);  
	  LCD_WriteReg(0x0000,0x0001);           
    LCD_WriteReg(0x0010,0x0000);       
    Delay(5);  
		
		LCD_WriteReg(0x0007,0x0233);       
		LCD_WriteReg(0x0011,0x6038);       
		LCD_WriteReg(0x0002,0x0600);       
		LCD_WriteReg(0x0003,0xA8A4);//0x0804  
		LCD_WriteReg(0x000C,0x0000);//
		LCD_WriteReg(0x000D,0x080C);//       
		LCD_WriteReg(0x000E,0x2900);       
		LCD_WriteReg(0x001E,0x00B8);       
		LCD_WriteReg(0x0001,0x293F);  //横屏
		LCD_WriteReg(0x0010,0x0000);       
		LCD_WriteReg(0x0005,0x0000);       
		LCD_WriteReg(0x0006,0x0000);       
		LCD_WriteReg(0x0016,0xEF1C);     
		LCD_WriteReg(0x0017,0x0003);     
		LCD_WriteReg(0x0007,0x0233);		//0x0233       
		LCD_WriteReg(0x000B,0x0000|(3<<6));  //////     
		LCD_WriteReg(0x000F,0x0000);		//
		LCD_WriteReg(0x0041,0x0000);     
		LCD_WriteReg(0x0042,0x0000);     
		LCD_WriteReg(0x0048,0x0000);     
		LCD_WriteReg(0x0049,0x013F);     
		LCD_WriteReg(0x004A,0x0000);     
		LCD_WriteReg(0x004B,0x0000);     
		LCD_WriteReg(0x0044,0xEF00);     
		LCD_WriteReg(0x0045,0x0000);     
		LCD_WriteReg(0x0046,0x013F);     
		LCD_WriteReg(0x0030,0x0707);     
		LCD_WriteReg(0x0031,0x0204);     
		LCD_WriteReg(0x0032,0x0204);     
		LCD_WriteReg(0x0033,0x0502);     
		LCD_WriteReg(0x0034,0x0507);     
		LCD_WriteReg(0x0035,0x0204);     
		LCD_WriteReg(0x0036,0x0204);     
		LCD_WriteReg(0x0037,0x0502);     
		LCD_WriteReg(0x003A,0x0302);     
		LCD_WriteReg(0x003B,0x0302);     
		LCD_WriteReg(0x0023,0x0000);     
		LCD_WriteReg(0x0024,0x0000);     
		LCD_WriteReg(0x0025,0x8000);     
		LCD_WriteReg(0x004e,0);        //?(X)??0
		LCD_WriteReg(0x004f,0);        //?(Y)??0

    Lcd_Light_ON;

    ili9320_Clear(BackColor);
//		Lcd_DisplayChinese16(0, 0,(const unsigned char *)brief);
//		Lcd_DisplayChinese16(0, 208,(const unsigned char *)addr);
//		Lcd_DisplayChinese16(0, 224,(const unsigned char *)teleph);
	//	Lcd_DisplayChinese32(0, 50,(const unsigned char *)"北京欢迎您！1234567890efghijklmnabc：合肥大明节能科技股份有限公司");

  //  Delay(1200);  
}

/****************************************************************************
* 名    称：void ili9320_SetWindows(u16 StartX,u16 StartY,u16 EndX,u16 EndY)
* 功    能：设置窗口区域
* 入口参数：StartX     行起始座标
*           StartY     列起始座标
*           EndX       行结束座标
*           EndY       列结束座标
* 出口参数：无
* 说    明：
* 调用方法：ili9320_SetWindows(0,0,100,100)；
****************************************************************************/
//inline void ili9320_SetWindows(u16 StartX,u16 StartY,u16 EndX,u16 EndY)
void ili9320_SetWindows(u16 StartX,u16 StartY,u16 EndX,u16 EndY)
{
  ili9320_SetCursor(StartX,StartY);
  LCD_WriteReg(0x0050, StartX);
  LCD_WriteReg(0x0052, StartY);
  LCD_WriteReg(0x0051, EndX);
  LCD_WriteReg(0x0053, EndY);
}

/******************************************************************************/

void ili9320_Darken(u8 row, u16 Color)
{
	u32 index=0;
	
	row = 14 + (row - 1) * 18;
	
  ili9320_SetCursor(row*320,0); 
  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(index=0; index<16*320; index++)
  {
		 if(Color != FontColor){
			LCD->LCD_RAM=Color;
		 }
  }
}

/****************************************************************************
* 名    称：u16 ili9320_BGR2RGB(u16 c)
* 功    能：RRRRRGGGGGGBBBBB 改为 BBBBBGGGGGGRRRRR 格式
* 入口参数：c      BRG 颜色值
* 出口参数：RGB 颜色值
* 说    明：内部函数调用
* 调用方法：
****************************************************************************/
u16 ili9320_BGR2RGB(u16 c)
{
  u16  r, g, b, rgb;

  b = (c>>0)  & 0x1f;
  g = (c>>5)  & 0x3f;
  r = (c>>11) & 0x1f;
  
  rgb =  (b<<11) + (g<<5) + (r<<0);

  return( rgb );
}

/****************************************************************************
* 名    称：u16 ili9320_GetPoint(u16 x,u16 y)
* 功    能：获取指定座标的颜色值
* 入口参数：x      行座标
*           y      列座标
* 出口参数：当前座标颜色值
* 说    明：
* 调用方法：i=ili9320_GetPoint(10,10);
****************************************************************************/
u16 ili9320_GetPoint(u16 x,u16 y)
{
  ili9320_SetCursor(x,y);
  return (ili9320_BGR2RGB(LCD_ReadRAM()));
}

/****************************************************************************
光标设置函数

*****************************************************************************/
//static unsigned char First_Dot = 0;  //定义第一行的第一个点的Y坐标

extern char JudgeMaxNum(void);

void ili9320_SetLight(char line){
	int index;
	u16 data;
	unsigned int msg;
	unsigned short divisor, remainder;
	
	data = line - 2;
	if(line == 1)
		data = JudgeMaxNum() - 1;
  msg = data * 16 * 320;
	
	for(index=0; index<(320 * 16); index++)
  {
	 divisor = (index + msg)/320;
	 remainder = index%320;
	 ili9320_SetCursor(remainder, divisor);
	 data = LCD_ReadRAM();
	 if(data != FontColor){
		ili9320_SetCursor(remainder, divisor);
		LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
		LCD->LCD_RAM = BackColor;
	 }
  }
	
	data = line;
	if(data > (JudgeMaxNum() - 1))
		data = 0;
  msg = data * 16 * 320;
	
	for(index=0; index<(320 * 16); index++)
  {
	 divisor = (index + msg)/320;
	 remainder = index%320;
	 ili9320_SetCursor(remainder, divisor);
	 data = LCD_ReadRAM();
	 if(data != FontColor){
		ili9320_SetCursor(remainder, divisor);
		LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
		LCD->LCD_RAM = BackColor;
	 }
  }
	
	line = line - 1;
  msg = line * 16 * 320;
	
  for(index=0; index<16*320; index++){	
	 divisor = (index + msg)/320;
	 remainder = index%320;
	 ili9320_SetCursor(remainder, divisor);
	 data = LCD_ReadRAM();
	 if(data != FontColor){
		ili9320_SetCursor(remainder, divisor);
		LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
		LCD->LCD_RAM = EnLight;
	 }
  }
}


/****************************************************************************
点亮一个数字


*****************************************************************************/

void ili9320_LightByte(char numb){
	int i, j, line, row, remain;
	u16 data;
	
	j = (numb - 1) * 8 % 320;         //列数
	i = (numb * 8 - 1) / 320 * 16;    //行数
	line = i + 16;                    //最大行数  
	row  = j + 8;                     //最大列数
	remain = j;
	
	for(; i < line; i++){
		for(j = remain; j < row; j++){
			ili9320_SetCursor(j, i);      // 先为列，后为行
			data = LCD_ReadRAM();
			if(data != FontColor){
				ili9320_SetCursor(j, i);
				LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
				LCD->LCD_RAM = TestColor;
			}
		}
	}
	
	if(numb > BaseBit + 1)
		j = (numb - 2) * 8 % 320;              //列数
	else
		j = (MaxBit + BaseBit - 1) * 8 % 320;
//	j = (numb - 1) * 8 % 320;         //列数
	i = (numb * 8 - 1) / 320 * 16;    //行数
	line = i + 16;                    //最大行数  
	row  = j + 8;                     //最大列数
	remain = j;

	for(; i < line; i++){
		for(j = remain; j < row; j++){
			ili9320_SetCursor(j, i);
			data = LCD_ReadRAM();
			if(data != FontColor){
				ili9320_SetCursor(j, i);                 
				LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
				LCD->LCD_RAM = BackColor;
			}
		}
	}
	
	if(numb < MaxBit + BaseBit)
		j = numb * 8 % 320;                   //列数
	else 
		j = 0;
//	j = (numb - 1) * 8 % 320;         //列数
	i = (numb * 8 - 1) / 320 * 16;    //行数
	line = i + 16;                    //最大行数  
	row  = j + 8;                     //最大列数
	remain = j;

	for(; i < line; i++){
		for(j = remain; j < row; j++){
			ili9320_SetCursor(j, i);
			data = LCD_ReadRAM();
			if(data != FontColor){
				ili9320_SetCursor(j, i);
				LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
				LCD->LCD_RAM = BackColor;
			}
		}
	}
}


/****************************************************************************
* 名    称：void ili9320_SetPoint(u16 x,u16 y,u16 point)
* 功    能：在指定座标画点
* 入口参数：x      行座标
*           y      列座标
*           point  点的颜色
* 出口参数：无
* 说    明：
* 调用方法：ili9320_SetPoint(10,10,0x0fe0);
****************************************************************************/
void ili9320_SetPoint(u16 x,u16 y,u16 point)
{
  if ( (x>320)||(y>240) ) return;
  ili9320_SetCursor(x,y);

  LCD_WriteRAM_Prepare();
  LCD_WriteRAM(point);
}


void TFT_SetXY(u16 x,u16 y)
{
  if ( (x>320)||(y>240) ) return;
  ili9320_SetCursor(x,y);

  LCD_WriteRAM_Prepare();
}

/****************************************************************************
* 名    称：void ili9320_PutChar(u16 x,u16 y,u8 c,u16 charColor,u16 bkColor)
* 功    能：在指定座标显示一个8x16点阵的ascii字符
* 入口参数：x          行座标
*           y          列座标
*           charColor  字符的颜色
*           bkColor    字符背景颜色
* 出口参数：无
* 说    明：显示范围限定为可显示的ascii码
* 调用方法：ili9320_PutChar(10,10,'a',0x0000,0xffff);
****************************************************************************/
void ili9320_PutChar(u16 x,u16 y,u8 c,u16 charColor,u16 bkColor)  // Lihao
{
  u16 i=0;
  u16 j=0;
  
  u8 tmp_char=0;
  
  if(HyalineBackColor == bkColor)
  {
    for (i=0;i<16;i++)
    {
      tmp_char=ascii_8x16[((c-0x20)*16)+i];
      for (j=0;j<8;j++)
      {
          if ( (tmp_char >> 7-j) & 0x01 == 0x01)
          {
              ili9320_SetPoint(x+j,y+i,charColor); // 字符颜色
          }
          else
          {
              // do nothing // 透明背景
          }
      }
    }
  }
  else
  {
        for (i=0;i<16;i++)
    {
      tmp_char=ascii_8x16[((c-0x20)*16)+i];
      for (j=0;j<8;j++)
      {
        if ( (tmp_char >> 7-j) & 0x01 == 0x01)
          {
            ili9320_SetPoint(x+j,y+i,charColor); // 字符颜色
          }
          else
          {
            ili9320_SetPoint(x+j,y+i,bkColor); // 背景颜色
          }
      }
    }
  }
}

/****************************************************************************
* 名    称：void ili9320_BackLight(u8 status)
* 功    能：开、关液晶背光
* 入口参数：status     1:背光开  0:背光关
* 出口参数：无
* 说    明：
* 调用方法：ili9320_BackLight(1);
****************************************************************************/
void ili9320_BackLight(u8 status)
{
  if ( status >= 1 )
  {
    Lcd_Light_ON;
  }
  else
  {
    Lcd_Light_OFF;
  }
}

void BackColorSet(void){
	ili9320_Clear(BackColor);
}

typedef enum{
	ILI_DISPLAY_PARAM,
	ILI_DISPLAY_NODE,
	ILI_LIGHT_BYTE,
	ILI_LIGHT_LINE,
	ILI_DISPLAY_FREQU,
	ILI_DISPLAY_INFOR,
	ILI_CHANGE_INTERFACE,
	ILI_INPUT_DISPALY,
	ILI_ORDER_DISPALY,
	ILI_CLEAR_SCREEN,
	ILI_UPANDDOWN_PAGE,
	ILI_NULL,
}Ili9320TaskMsgType;

typedef struct {
	/// Message type.
	Ili9320TaskMsgType type;
	/// Message lenght.
	unsigned short length;
} Ili9320TaskMsg;

static Ili9320TaskMsg *__Ili9320CreateMessage(Ili9320TaskMsgType type, const char *dat, int len) {
  Ili9320TaskMsg *message = pvPortMalloc(ALIGNED_SIZEOF(Ili9320TaskMsg) + len);
	if (message != NULL) {
		message->type = type;
		message->length = len;
		memcpy(&message[1], dat, len);
	}
	return message;
}

static inline void *__Ili9320GetMsgData(Ili9320TaskMsg *message) {
	if(message->length != 0)
		return &message[1];
	else
		return NULL;
}

bool Ili9320TaskDisLightParam(const char *dat, int len){                               //显示选择的灯参数
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_DISPLAY_PARAM, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
	
}

bool Ili9320TaskDisNode(const char *dat, int len){                               //显示中心节点频点和网络ID
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_DISPLAY_NODE, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
	
}

bool Ili9320TaskDisFrequDot(const char *dat, int len){                               //显示双频点数据信息
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_DISPLAY_FREQU, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
	
}

bool Ili9320TaskDisGateWay(const char *dat, int len) {                              //项目、网关、频点信息显示
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_DISPLAY_INFOR, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskLightByte(const char *dat, int len) {                              //点亮一个字背景
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_LIGHT_BYTE, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskLightLine(const char *dat, int len) {                              //点亮一行背景
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_LIGHT_LINE, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskChangeInterface(const char *dat, int len) {                                              
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_CHANGE_INTERFACE, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskInputDis(const char *dat, int len) {                               //高级设置中，输入法切换
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_INPUT_DISPALY, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskOrderDis(const char *dat, int len) {                               //正常显示
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_ORDER_DISPALY, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskClear(const char *dat, int len) {                                 //	清屏
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_CLEAR_SCREEN, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

bool Ili9320TaskUpAndDown(const char *dat, int len) {                              //高级设置中，上下页切换    
	Ili9320TaskMsg *message = __Ili9320CreateMessage(ILI_UPANDDOWN_PAGE, dat, len);
	if (pdTRUE != xQueueSend(__Ili9320Queue, &message, configTICK_RATE_HZ * 5)) {
		vPortFree(message);
		return true;
	}
	return false;
}

char Line = 1;                               //当前显示页面显示到的行数

static char LastLine = 1;                     //记录上次显示屏显示到的行数
static const uint8_t *__displayNewPage = NULL;				 //记录当前页面显示不完的数据指针		

static const uint8_t *__displayLastPage = NULL;        //记录上一页面显示的数据指针
static const uint8_t *__displayCurrentPage = NULL;     //记录当前页面显示的数据指针

static unsigned short len = 0;                         //显示的数据与显示指针的偏移量

void ili9320_ClearLine(void)
{
  u32 index=0;
  ili9320_SetCursor(0,(Line - 1) * 16); 
  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(index=0;index<5120;index++)
   {
     LCD->LCD_RAM=BackColor;
   }
}

void __HandleLightPara(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	
	Lcd_DisplayLightParam(0, 160, (const unsigned char *)p);
}

void __HandleDisplayFrequ(Ili9320TaskMsg *msg){
	char buf[128];
	
	sprintf(buf, "第一频点:%02X    网络ID:%02X\r\n第二频点:%02X    网络ID:%02X", FrequPoint1, NetID1,FrequPoint2, NetID2);
	
	Lcd_DisplayDeal(0, 64, (const unsigned char *)buf);
}

void __HandleInput(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	Lcd_DisplayInput(296, 224, (const unsigned char *)p);
}

void __HandleDisChoice(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	Lcd_DisplayDeal(0, 208, (const unsigned char *)p);
}

void __HandleDisNode(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	
	Lcd_DisplayNode(0, 192, (const unsigned char *)p);
}

void __HandleUpAndDown(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	
	BackColorSet();
	if(strncmp(p, "UP", 2) == 0){
		Lcd_LineDisplay16(1, (const unsigned char *)__displayLastPage);
	} else if(strncmp(p, "DOWN", 4) == 0)
		Lcd_LineDisplay16(1, (const unsigned char *)__displayCurrentPage);
}

void __HandleOrderDisplay(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);

	if((Line == 1) && (InterFace != Light_Attrib)){
		BackColorSet();
	}
	
	ili9320_ClearLine();
	__displayNewPage = Lcd_LineDisplay16(Line, (const unsigned char *)p);
	
	if(LastLine <= Line){
		if(strlen(p) > 4)
			len += sprintf((char *)__displayCurrentPage + len, "%s\r\n", p);
		LastLine = Line;
		if(__displayNewPage != NULL){
			len = 0;
			strcpy((char *)__displayLastPage,  (const char *)__displayCurrentPage);	
			len += sprintf((char *)__displayCurrentPage + len, "%s\r\n", __displayNewPage);
		}
	} else {	
		if(Line == 1){
			len = 0;
			strcpy((char *)__displayLastPage,  (const char *)__displayCurrentPage);		
			if(strlen(p) > 4)
				len = sprintf((char *)__displayCurrentPage + len, "%s\r\n", p);
		}
		LastLine = 1;
	}
	
	if(__displayNewPage != NULL){
		BackColorSet();
		Lcd_LineDisplay16(1, (const unsigned char *)__displayNewPage);
	}
	
	if((strlen(p) > 4) && (strlen(p) <= 40)){
		Line++;
	} else if(strlen(p) > 40){
		Line += (strlen(p) -1)/40 + 1;
	
	} else if(strlen(p) < 5){
		return;
	}
	
	if(Line > 15){
		Line = Line - 15;
	}

}

void __HandleCls(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	
	if(p[0] == 'C'){
		BackColorSet();		
		Line = 1;
	} else{
		TFT_ClearLine(p[0] - '0');
		Line = p[0] - '0' + 1;
	}
}



void __HandleChangeInterface(Ili9320TaskMsg *msg){

}

void __HandleLightLine(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	
	ili9320_SetLight(p[0]);
}


void __HandleLightByte(Ili9320TaskMsg *msg){
	char *p = __Ili9320GetMsgData(msg);
	
	ili9320_LightByte(p[0]);
}

typedef struct {
	Ili9320TaskMsgType type;
	void (*handlerFunc)(Ili9320TaskMsg *);
} MessageHandlerMap;


static const MessageHandlerMap __messageHandlerMaps[] = {
	{ ILI_DISPLAY_PARAM, __HandleLightPara },
	{ ILI_DISPLAY_NODE,  __HandleDisNode },
	{ ILI_DISPLAY_FREQU, __HandleDisplayFrequ },
	{ ILI_LIGHT_BYTE, __HandleLightByte },
	{ ILI_LIGHT_LINE, __HandleLightLine },
	{ ILI_DISPLAY_INFOR, __HandleDisChoice },
	{ ILI_CHANGE_INTERFACE, __HandleChangeInterface},
	{ ILI_UPANDDOWN_PAGE, __HandleUpAndDown},
	{ ILI_INPUT_DISPALY, __HandleInput},
	{ ILI_ORDER_DISPALY, __HandleOrderDisplay },
	{ ILI_CLEAR_SCREEN, __HandleCls },
	{ ILI_NULL, NULL },
};


static void __Ili9320Task(void *parameter) {
	portBASE_TYPE rc;
	Ili9320TaskMsg *message;

	__displayLastPage = pvPortMalloc(640);
	__displayCurrentPage = pvPortMalloc(640);
	for (;;) {
	//	printf("ili9320: loop again\n");
		rc = xQueueReceive(__Ili9320Queue, &message, configTICK_RATE_HZ);
		if (rc == pdTRUE) {
			const MessageHandlerMap *map = __messageHandlerMaps;
			printf("%s", (message + 2));
			for (; map->type != ILI_NULL; ++map) {
				if (message->type == map->type) {
					map->handlerFunc(message);
					break;
				}
			}
			vPortFree(message);
		} 
	}
}


void Ili9320Init(void) {
	ili9320_Initializtion();
	__Ili9320Queue = xQueueCreate(50, sizeof(Ili9320TaskMsg *));
	xTaskCreate(__Ili9320Task, (signed portCHAR *) "ILI9320", ILI_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5, NULL);
}

