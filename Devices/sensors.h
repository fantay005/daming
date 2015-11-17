#ifndef __SENSORS_H__
#define __SENSORS_H__

/*
光照传感器:
Power:		3.3v
PB6(42)		SDA	(4.7k上拉)
PB7(43)		SCL	(4.7k上拉)
][
土壤温度:
Power:		5v
PA2(12)		DQ
PA3(13)		DQ
PA4(14)		DQ(4.7k上拉)

土壤湿度:
Power:		5v
PA0:(10)	ADC12_INT0
PA6:(16)	ADC12_INT6
PB0:(18)	ADC12_INT8

风向:		100欧姆电阻
Power:		12v
PA7:(17)	ADC12_INT7
**
风速:
Power:		12v
PA1(11)		TIM2

雨量:												                                            
Power:		5v
PB8(45)		EXTI    (10K上拉)

空气温湿度:
Power:		3.3v
PB2(20)		SCL		(4.7k上拉)
PB1(19)		SDA		(4.7k上拉)
*/
void SensorManager_config(void);
void Sensor_Notify(const char *dat, int size);

#endif

