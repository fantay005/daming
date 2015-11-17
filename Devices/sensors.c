#include "sensors.h"
#include "bh1750.h"
#include "soil.h"
#include "rainfall.h"
#include "windsensor.h"
#include "sht1x.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "adc_dma.h"
#include "misc.h"

#define MAX_SIZE_BUF	256

static uint32_t SN;

void __sensors_config(void)
{
	TimDelay_Config();
	ADCx_DMA_Config();
	BH1750_config();		     // 光照传感器 
	SoilSensor_Config();         // 土壤传感器 
//	Rainfall_Config();			 // 雨量传感器
//	WindSensor_Config();		 // 风传感器
	SHT10Init();				 // 空气温湿度
}

//static
// void __package(const char *title, uint32_t dat)
// {
// 	static char buffer[64];
// 	int len = sprintf(buffer, "#HOT\t%s=%d\tOV\n\r",title, dat);
// //	Comm_SendToUart(buffer, len, IN_TASK);
// }

char *backdate(int *size) {
			int dat;
			int val;
	    char *buffer;
	    int len = 0;
			buffer = pvPortMalloc(MAX_SIZE_BUF);
			len = sprintf(buffer, "#HXC_SN%d_",SN++);
			
			dat = BH1750_Read();
			len += sprintf(&buffer[len], "BH%d_", dat);

			dat = SoilTemp_Read(0);
			len += sprintf(&buffer[len], "TSA%d_", dat);
 			
// 			dat = SoilTemp_Read(1);
// 			len += sprintf(&buffer[len], "TSB%d_", dat);
// 			
// 			dat = SoilTemp_Read(2);
// 			len += sprintf(&buffer[len], "TSC%d_", dat);

			dat = SoilHum_Read(0);
			len += sprintf(&buffer[len], "HSA%d_", dat);
			
			dat = SoilHum_Read(1);
			len += sprintf(&buffer[len], "HSB%d_", dat);
			
			dat = SoilHum_Read(2);
			len += sprintf(&buffer[len], "HSC%d_", dat);
	
			SHT10ReadTemperatureHumidity(&dat, &val);
			len += sprintf(&buffer[len], "TA%d_", dat);
			len += sprintf(&buffer[len], "HA%d_", val);

			len += sprintf(&buffer[len], "OV\n\r");
			*size = len;
      return buffer;
}

