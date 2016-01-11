#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "rtc.h"
#include "second_datetime.h"
#include "math.h"
#include "time.h"
#include "norflash.h"
#include "protocol.h"
#include "gsm.h"

#define SHT_TASK_STACK_SIZE	( configMINIMAL_STACK_SIZE + 1024)

#define DetectionTime  1

#define M_PI 3.14
#define RAD  (180.0 * 3600 / M_PI)

static double richu;
static double midDayTime;
static double dawnTime;
static double jd;
static double wd;
static unsigned char sunup[3] = {0};
static unsigned char sunset[3] = {0};
static unsigned char daybreak[3] = {0};
static unsigned char daydark[3] = {0};

static unsigned char SunTime = 0;

unsigned char CurrentTime(void){
	return SunTime;
}

/*************************
     * �����յļ���
     *
     * @param y ��
     *
     * @param M ��
     *
     * @param d ��
     *
     * @param h ʱ
     *
     * @param m ��
     *
     * @param s ��
     *
     * @return int
***************************/
static double timeToDouble(int y, int M, double d){
//  double A=0;
		double B=0;
		double jd=0;

		/*��yΪ������ݣ�MΪ�·ݣ�DΪ��Լ����*/
		/*��M > 2�� Y��M���䣬��M = 1��2����y - 1��y����M + 12��M�����仰˵�����������1�»�2��
			�򱻿�������ǰһ���13�»�14��*/
		/*�Ը���������У�A = INT(Y/100)   B = 2 - A + INT(A/4)*/
		/*����������ȡB = 0*/
		/*JD = INT(365.25(Y+4716))+INT(30.6001(M+1))+D+B-1524.5 (7.1)*/
		B=-13;
		jd=floor(365.25 * (y + 4716))+ floor(30.60001 * (M + 1)) +B+ d- 1524.5;
		return jd;
}

static void doubleToTime(double time, unsigned char p[3]){
	 double t;
	 unsigned char h,m,s;

		t = time + 0.5;
		t = (t - (int) t) * 24;
		h = (unsigned char) t;
		t = (t - h) * 60;
		m = (unsigned char) t;
		t = (t - m) * 60;
		s = (unsigned char) t;
	  p[0] = h;
		p[1]  = m;
		p[2]  = s;
}

/****************************
     * @param t ����������
     *
     * @return ̫���ƾ�
*****************************/
static double sunHJ(double t){
		double j;
		t = t + (32.0 * (t + 1.8) * (t + 1.8) - 20) / 86400.0 / 36525.0;
		/*����������������ѧʱ*/
		j = 48950621.66 + 6283319653.318 * t + 53 * t * t - 994 + 334166 *cos(4.669257 + 628.307585 * t) + 3489 * cos(4.6261 + 1256.61517 * t) + 2060.6 * cos(2.67823 + 628.307585 * t) * t;
		return (j / 10000000);
}

static double mod(double num1, double num2){
		num2 = fabs(num2);
		/*ֻ��ȡ����Num1�ķ���*/
		return num1 >= 0 ?(num1 - (floor(num1 / num2)) * num2 ): ((floor(fabs(num1) / num2)) * num2 - fabs(num1));
}
/********************************
     * ��֤�ǶȦ� (-��, ��)
     *
     * @param ag
     * @return ag
***********************************/
static double degree(double ag){
		ag = mod(ag, 2 * M_PI);
		if(ag<=-M_PI){
				ag=ag+2*M_PI;
		}
		else if(ag>M_PI){
				ag=ag-2*M_PI;
		}
		return ag;
}

/***********************************
     *
     * @param date  ������ƽ��
     *
     * @param lo    ������
     *
     * @param la    ����γ��
     *
     * @param tz    ʱ��
     *
     * @return ̫������ʱ��
*************************************/
double sunRiseTime(double date, double lo, double la, double tz) {
		double t,j,sinJ,cosJ,gst,E,a,D,cosH0,cosH1,H0,H1,H;
		date = date - tz;
		/*̫���ƾ��Լ�����������ֵ*/
		t = date / 36525;
		j = sunHJ(t);
		/*̫���ƾ��Լ�����������ֵ*/
		sinJ = sin(j);
		cosJ = cos(j);
		/*����2*M_PI*(0.7790572732640 + 1.00273781191135448*jd)����ʱ������Ȧλ�ã�*/
		gst = 2 * M_PI * (0.779057273264 + 1.00273781191135 * date) + (0.014506 + 4612.15739966 * t + 1.39667721 * t * t) / RAD;
		E = (84381.406 - 46.836769 * t) / RAD; /*�Ƴཻ��*/
		a = atan2(sinJ * cos(E), cosJ);/*̫���ྭ*/
		D = asin(sin(E) * sinJ); /*̫����γ*/
		cosH0 = (sin(-50 * 60 / RAD) - sin(la) * sin(D)) / (cos(la) * cos(D)); /*�ճ���ʱ�Ǽ��㣬��ƽ����50��*/
		cosH1 = (sin(-6 * 3600 / RAD) - sin(la) * sin(D)) / (cos(la) * cos(D)); /*������ʱ�Ǽ��㣬��ƽ����6�ȣ���Ϊ�������Ϊ��ƽ����12��*/
		/*�ϸ�Ӧ�����ּ��缫ҹ����������*/
		if (cosH0 >= 1 || cosH0 <= -1){
				return -0.5;/*����*/
		}
		H0 = -acos(cosH0); /*����ʱ�ǣ��ճ�����ȥ�����ţ����ǽ���ʱ�ǣ�Ҳ��������������������*/
		H1 = -acos(cosH1);
		H = gst - lo - a; /*̫��ʱ��*/
		midDayTime = date - degree(H) / M_PI / 2 + tz; /*����ʱ��*/
		dawnTime = date - degree(H - H1) / M_PI / 2 + tz;/*����ʱ��*/
		return date - degree(H - H0) / M_PI / 2 + tz; /*�ճ�ʱ�䣬��������ֵ*/
}

extern unsigned char *ProtocolMessage(unsigned char address[10], unsigned char  type[2], const char *msg, unsigned char *size);

static void __TimeTask(void *nouse) {
	DateTime dateTime;
	uint32_t second;
	unsigned int i;
	GatewayParam1 g;
	double jd_degrees = 117;
  double jd_seconds = 17;
  double wd_degrees = 31;
  double wd_seconds = 52;
	static unsigned char FLAG = 0, Anti = 0;
	portTickType curT, lastT = 0;
	int NowTime, DayTime, DarkTime;
		 
	while (1) {
		 if (!RtcWaitForSecondInterruptOccured(portMAX_DELAY)) {
			continue;
		 }	 
		 
		 if(Anti == 0){                         /*�����Ƿ����о�γ������*/
			 NorFlashRead(NORFLASH_MANAGEM_BASE, (short * )&g, (sizeof(GatewayParam1) + 1) / 2);
			 if(g.EmbedInformation == 1){         /*��Flash�ж������ز���*/ 
				 unsigned char msg[8];
				 
				 sscanf((const char *)&(g.Longitude), "%*1s%3s", msg);
				 jd_degrees = atoi((const char *)msg);
				 sscanf((const char *)&(g.Longitude), "%*4s%2s", msg);
				 jd_seconds = atoi((const char *)msg);

				 sscanf((const char *)&(g.Latitude), "%*1s%3s", msg);
				 wd_degrees = atoi((const char *)msg);
				 sscanf((const char *)&(g.Latitude), "%*4s%2s", msg);
				 wd_seconds = atoi((const char *)msg);
				 Anti = 1;
			 }
		 }
		 
		 second = RtcGetTime();
		 SecondToDateTime(&dateTime, second);
		 
		 if((dateTime.year < 0x10) || (dateTime.month > 0x0C) || (dateTime.date > 0x1F) || (dateTime.hour > 0x3D)) {  /*ʱ�������쳣*/
			 unsigned char *buf, size;
			 
			 curT = xTaskGetTickCount();
			 
			 if((curT - lastT) > configTICK_RATE_HZ * 60){
				 buf = ProtocolMessage("9999999999", "45", NULL, &size);
				 GsmTaskSendTcpData((const char *)buf, size);
				 vPortFree(buf);
				 lastT = curT;
			 }
			 continue;
		 }
		 
//		 printf("%d.\r\n", dateTime.year);
		 if ((FLAG == 0) && (dateTime.second != 0x00)){
			
				jd = -(jd_degrees + jd_seconds / 60) / 180 * M_PI;
				wd = (wd_degrees + wd_seconds / 60) / 180 * M_PI;
				richu = timeToDouble(dateTime.year + 2000, dateTime.month, (double)dateTime.date) - 2451544.5;
				for (i = 0; i < 10; i++){
					richu = sunRiseTime(richu, jd, wd, 8/24.0);/*�𲽱ƽ��㷨10��*/
				}
				doubleToTime(richu, sunup);
				if((sunup[0] > 8) && (sunup[0] < 3)){
					continue;
				}
				
				if(sunup[1] > 59){
					continue;
				}
				
				if(sunup[2] > 59){
					continue;
				}
				
				doubleToTime(midDayTime + midDayTime - richu, sunset);
				
				if((sunset[0] > 21) && (sunset[0] < 17)){
					continue;
				}
				
				if(sunset[1] > 59){
					continue;
				}
				
				if(sunset[2] > 59){
					continue;
				}
				doubleToTime(dawnTime, daybreak);
				doubleToTime(midDayTime + midDayTime - dawnTime, daydark);	
				
				FLAG = 1;
			
				
		}	else if ((dateTime.hour == 0x00) && (dateTime.minute == 0x01) && (dateTime.second == 0x00)) {
			
			FLAG = 0;
			vTaskDelay(configTICK_RATE_HZ);		
		} else if((dateTime.hour == 0x0C)&& (dateTime.minute == 0x0F) && (dateTime.second == 0x00)){
			
			NVIC_SystemReset();			
		}
		
		if(FLAG == 1){
			
			NowTime = dateTime.hour * 60 * 60 + dateTime.minute * 60 + dateTime.second;    /*��ǰʱ��*/
			DayTime = daybreak[0] * 60 * 60 + daybreak[1] * 60 + daybreak[2];              /*����ʱ��*/
			DarkTime = daydark[0] * 60 * 60 + daydark[1] * 60 + daydark[2];                /*���ʱ��*/
			
			if((NowTime > DayTime) && (NowTime < DarkTime))                                /*����ʱ��*/
				SunTime = 1;
			else
				SunTime = 0;
		}
		
	}
}

void TimePlanInit(void) {
	xTaskCreate(__TimeTask, (signed portCHAR *) "TEST", SHT_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}


