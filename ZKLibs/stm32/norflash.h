#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include <stdbool.h>
#include <stdint.h>
#include "fsmc_nor.h"

/********************************************************************************************************
* NOR_FLASH��ַӳ���
********************************************************************************************************/
//NOR_FLASH
#define NORFLASH_SECTOR_SIZE   				          ((uint32_t)0x00001000)/*һ�������Ĵ�С*/  
#define NORFLASH_BLOCK_SIZE                     ((uint32_t)0x00010000)/*һ��Ĵ�С*/
#define NORFLASH_MANAGEM_BASE  				          ((uint32_t)0x00001000)/*���ز���-������ݱ�ʶ����γ�ȡ�ZIGBEEƵ�㡢�Զ��ϴ�����ʱ����*/
#define NORFLASH_BALLAST_NUM   				          ((uint32_t)0x00002000)/*��������Ŀ*/
#define NORFLASH_CHIP_ERASE                     ((uint32_t)0x00005000)/*D*/ 
#define NORFLASH_STRATEGY_ADDR                  ((uint32_t)0x00006000)/*���������������ͳһ���Է��õ�ַ*/


#define NORFLASH_LIGHT_NUMBER                   ((uint32_t)0x00009000)/*��һ��short��������λ���صĵƲ������������Ƶĵ��������ڶ���short��������Ϊ����zigbee��ַ*/
#define NORFLAH_ERASE_BLOCK_BASE                ((uint32_t)0x00010000)/*��������ʼ��ַ*/
#define NORFLASH_BALLAST_BASE  				          ((uint32_t)0x00018000)/*Zigbee1 ������������ַ*/
#define NORFLASH_BSN_PARAM_BASE                 ((uint32_t)0x00218000)/*Zigbee2 ������������ַ*/
#define NORFLASH_MANAGEM_ADDR                   ((uint32_t)0x00400000)/*���ص�ַ��������IP��ַ���˿ں�*/
#define NORFLASH_MANAGEM_TIMEOFFSET 		        ((uint32_t)0x00401000)/*���ز���-����ƫ�ơ��ص�ƫ��*/
#define NORFLASH_MANAGEM_WARNING   		          ((uint32_t)0x00402000)/*���ز���-�澯*/
#define NORFLASH_RESET_TIME           		    	((uint32_t)0x00404000)/*����ʱ��*/
#define NORFLASH_RESET_COUNT                    ((uint32_t)0x00405000)/*��������*/
#define NORFLASH_ELEC_UPDATA_TIME               ((uint32_t)0x00406000)/*�����ϴ�ʱ��*/
#define NORFLASH_PARAM_OFFSET   				        ((uint32_t)0x00001000)
#define NORFLASH_STRATEGY_OFFSET        		    ((uint32_t)0x00001000)

/*��������ز�����������������ƫ��*/
#define PARAM_LUX_DOG_OFFSET                    ((uint32_t)0x00417000)/*���ն����򻮷ֵ����*/              
#define PARAM_TIME_DOG_OFFSET                   ((uint32_t)0x00418000)/*ʱ�����򻮷ֵ����*/

#define SECTION_SPACE_SIZE                      ((uint32_t)0x00020000)/*�δ洢����ռ��С*/

/*�����β��Դ洢*/
#define STRATEGY_GUIDE_ONOFF_DAZZLING           ((uint32_t)0x00420000)/*�����Σ����ص�ʱ��Σ�ǿ�������*/ 
#define STRATEGY_GUIDE_ONOFF_BRIGHT             ((uint32_t)0x00421000)/*�����Σ����ص�ʱ��Σ��й������*/ 
#define STRATEGY_GUIDE_ONOFF_VISIBLE            ((uint32_t)0x00422000)/*�����Σ����ص�ʱ��Σ����������*/ 
#define STRATEGY_GUIDE_ONOFF_UNSEEN             ((uint32_t)0x00423000)/*�����Σ����ص�ʱ��Σ��޹������*/ 
#define STRATEGY_GUIDE_DAY_DAZZLING             ((uint32_t)0x00424000)/*�����Σ�����ʱ��Σ�ǿ�������*/ 
#define STRATEGY_GUIDE_DAY_BRIGHT               ((uint32_t)0x00425000)/*�����Σ�����ʱ��Σ��й������*/ 
#define STRATEGY_GUIDE_DAY_VISIBLE              ((uint32_t)0x00426000)/*�����Σ�����ʱ��Σ����������*/ 
#define STRATEGY_GUIDE_DAY_UNSEEN               ((uint32_t)0x00427000)/*�����Σ�����ʱ��Σ��޹������*/ 
#define STRATEGY_GUIDE_NIGHT_DAZZLING           ((uint32_t)0x00428000)/*�����Σ�ҹ��ʱ��Σ�ǿ�������*/ 
#define STRATEGY_GUIDE_NIGHT_BRIGHT             ((uint32_t)0x00429000)/*�����Σ�ҹ��ʱ��Σ��й������*/ 
#define STRATEGY_GUIDE_NIGHT_VISIBLE            ((uint32_t)0x0042A000)/*�����Σ�ҹ��ʱ��Σ����������*/ 
#define STRATEGY_GUIDE_NIGHT_UNSEEN             ((uint32_t)0x0042B000)/*�����Σ�ҹ��ʱ��Σ��޹������*/ 
#define STRATEGY_GUIDE_MIDNIGHT_DAZZLING        ((uint32_t)0x0042C000)/*�����Σ���ҹʱ��Σ�ǿ�������*/ 
#define STRATEGY_GUIDE_MIDNIGHT_BRIGHT          ((uint32_t)0x0042D000)/*�����Σ���ҹʱ��Σ��й������*/ 
#define STRATEGY_GUIDE_MIDNIGHT_VISIBLE         ((uint32_t)0x0042E000)/*�����Σ���ҹʱ��Σ����������*/ 
#define STRATEGY_GUIDE_MIDNIGHT_UNSEEN          ((uint32_t)0x0042F000)/*�����Σ���ҹʱ��Σ��޹������*/ 

/*��ڶβ��Դ洢*/
#define STRATEGY_ENTRY_ONOFF_DAZZLING           ((uint32_t)0x00440000)/*��ڶΣ����ص�ʱ��Σ�ǿ�������*/ 
#define STRATEGY_ENTRY_ONOFF_BRIGHT             ((uint32_t)0x00441000)/*��ڶΣ����ص�ʱ��Σ��й������*/ 
#define STRATEGY_ENTRY_ONOFF_VISIBLE            ((uint32_t)0x00442000)/*��ڶΣ����ص�ʱ��Σ����������*/ 
#define STRATEGY_ENTRY_ONOFF_UNSEEN             ((uint32_t)0x00443000)/*��ڶΣ����ص�ʱ��Σ��޹������*/ 
#define STRATEGY_ENTRY_DAY_DAZZLING             ((uint32_t)0x00444000)/*��ڶΣ�����ʱ��Σ�ǿ�������*/ 
#define STRATEGY_ENTRY_DAY_BRIGHT               ((uint32_t)0x00445000)/*��ڶΣ�����ʱ��Σ��й������*/ 
#define STRATEGY_ENTRY_DAY_VISIBLE              ((uint32_t)0x00446000)/*��ڶΣ�����ʱ��Σ����������*/ 
#define STRATEGY_ENTRY_DAY_UNSEEN               ((uint32_t)0x00447000)/*��ڶΣ�����ʱ��Σ��޹������*/ 
#define STRATEGY_ENTRY_NIGHT_DAZZLING           ((uint32_t)0x00448000)/*��ڶΣ�ҹ��ʱ��Σ�ǿ�������*/ 
#define STRATEGY_ENTRY_NIGHT_BRIGHT             ((uint32_t)0x00449000)/*��ڶΣ�ҹ��ʱ��Σ��й������*/ 
#define STRATEGY_ENTRY_NIGHT_VISIBLE            ((uint32_t)0x0044A000)/*��ڶΣ�ҹ��ʱ��Σ����������*/ 
#define STRATEGY_ENTRY_NIGHT_UNSEEN             ((uint32_t)0x0044B000)/*��ڶΣ�ҹ��ʱ��Σ��޹������*/ 
#define STRATEGY_ENTRY_MIDNIGHT_DAZZLING        ((uint32_t)0x0044C000)/*��ڶΣ���ҹʱ��Σ�ǿ�������*/ 
#define STRATEGY_ENTRY_MIDNIGHT_BRIGHT          ((uint32_t)0x0044D000)/*��ڶΣ���ҹʱ��Σ��й������*/ 
#define STRATEGY_ENTRY_MIDNIGHT_VISIBLE         ((uint32_t)0x0044E000)/*��ڶΣ���ҹʱ��Σ����������*/ 
#define STRATEGY_ENTRY_MIDNIGHT_UNSEEN          ((uint32_t)0x0044F000)/*��ڶΣ���ҹʱ��Σ��޹������*/ 

/*��ڹ��ɶβ��Դ洢*/
#define STRATEGY_TRANSIT_I_ONOFF_DAZZLING       ((uint32_t)0x00460000)/*��ڹ��ɶΣ����ص�ʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_I_ONOFF_BRIGHT         ((uint32_t)0x00461000)/*��ڹ��ɶΣ����ص�ʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_I_ONOFF_VISIBLE        ((uint32_t)0x00462000)/*��ڹ��ɶΣ����ص�ʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_I_ONOFF_UNSEEN         ((uint32_t)0x00463000)/*��ڹ��ɶΣ����ص�ʱ��Σ��޹������*/ 
#define STRATEGY_TRANSIT_I_DAY_DAZZLING         ((uint32_t)0x00464000)/*��ڹ��ɶΣ�����ʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_I_DAY_BRIGHT           ((uint32_t)0x00465000)/*��ڹ��ɶΣ�����ʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_I_DAY_VISIBLE          ((uint32_t)0x00466000)/*��ڹ��ɶΣ�����ʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_I_DAY_UNSEEN           ((uint32_t)0x00467000)/*��ڹ��ɶΣ�����ʱ��Σ��޹������*/ 
#define STRATEGY_TRANSIT_I__NIGHT_DAZZLING      ((uint32_t)0x00468000)/*��ڹ��ɶΣ�ҹ��ʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_I__NIGHT_BRIGHT        ((uint32_t)0x00469000)/*��ڹ��ɶΣ�ҹ��ʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_I__NIGHT_VISIBLE       ((uint32_t)0x0046A000)/*��ڹ��ɶΣ�ҹ��ʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_I__NIGHT_UNSEEN        ((uint32_t)0x0046B000)/*��ڹ��ɶΣ�ҹ��ʱ��Σ��޹������*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_DAZZLING   ((uint32_t)0x0046C000)/*��ڹ��ɶΣ���ҹʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_BRIGHT     ((uint32_t)0x0046D000)/*��ڹ��ɶΣ���ҹʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_VISIBLE    ((uint32_t)0x0046E000)/*��ڹ��ɶΣ���ҹʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_UNSEEN     ((uint32_t)0x0046F000)/*��ڹ��ɶΣ���ҹʱ��Σ��޹������*/ 

/*�м�β��Դ洢*/
#define STRATEGY_MIDDLE_ONOFF_DAZZLING          ((uint32_t)0x00480000)/*�м�Σ����ص�ʱ��Σ�ǿ�������*/ 
#define STRATEGY_MIDDLE_ONOFF_BRIGHT            ((uint32_t)0x00481000)/*�м�Σ����ص�ʱ��Σ��й������*/ 
#define STRATEGY_MIDDLE_ONOFF_VISIBLE           ((uint32_t)0x00482000)/*�м�Σ����ص�ʱ��Σ����������*/ 
#define STRATEGY_MIDDLE_ONOFF_UNSEEN            ((uint32_t)0x00483000)/*�м�Σ����ص�ʱ��Σ��޹������*/ 
#define STRATEGY_MIDDLE_DAY_DAZZLING            ((uint32_t)0x00484000)/*�м�Σ�����ʱ��Σ�ǿ�������*/ 
#define STRATEGY_MIDDLE_DAY_BRIGHT              ((uint32_t)0x00485000)/*�м�Σ�����ʱ��Σ��й������*/ 
#define STRATEGY_MIDDLE_DAY_VISIBLE             ((uint32_t)0x00486000)/*�м�Σ�����ʱ��Σ����������*/ 
#define STRATEGY_MIDDLE_DAY_UNSEEN              ((uint32_t)0x00487000)/*�м�Σ�����ʱ��Σ��޹������*/ 
#define STRATEGY_MIDDLE_NIGHT_DAZZLING          ((uint32_t)0x00488000)/*�м�Σ�ҹ��ʱ��Σ�ǿ�������*/ 
#define STRATEGY_MIDDLE_NIGHT_BRIGHT            ((uint32_t)0x00489000)/*�м�Σ�ҹ��ʱ��Σ��й������*/ 
#define STRATEGY_MIDDLE_NIGHT_VISIBLE           ((uint32_t)0x0048A000)/*�м�Σ�ҹ��ʱ��Σ����������*/ 
#define STRATEGY_MIDDLE_NIGHT_UNSEEN            ((uint32_t)0x0048B000)/*�м�Σ�ҹ��ʱ��Σ��޹������*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_DAZZLING       ((uint32_t)0x0048C000)/*�м�Σ���ҹʱ��Σ�ǿ�������*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_BRIGHT         ((uint32_t)0x0048D000)/*�м�Σ���ҹʱ��Σ��й������*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_VISIBLE        ((uint32_t)0x0048E000)/*�м�Σ���ҹʱ��Σ����������*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_UNSEEN         ((uint32_t)0x0048F000)/*�м�Σ���ҹʱ��Σ��޹������*/ 

/*���ڹ��ɶβ��Դ洢*/
#define STRATEGY_TRANSIT_II_ONOFF_DAZZLING      ((uint32_t)0x004A0000)/*���ڹ��ɶΣ����ص�ʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_II_ONOFF_BRIGHT        ((uint32_t)0x004A1000)/*���ڹ��ɶΣ����ص�ʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_II_ONOFF_VISIBLE       ((uint32_t)0x004A2000)/*���ڹ��ɶΣ����ص�ʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_II_ONOFF_UNSEEN        ((uint32_t)0x004A3000)/*���ڹ��ɶΣ����ص�ʱ��Σ��޹������*/ 
#define STRATEGY_TRANSIT_II_DAY_DAZZLING        ((uint32_t)0x004A4000)/*���ڹ��ɶΣ�����ʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_II_DAY_BRIGHT          ((uint32_t)0x004A5000)/*���ڹ��ɶΣ�����ʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_II_DAY_VISIBLE         ((uint32_t)0x004A6000)/*���ڹ��ɶΣ�����ʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_II_DAY_UNSEEN          ((uint32_t)0x004A7000)/*���ڹ��ɶΣ�����ʱ��Σ��޹������*/ 
#define STRATEGY_TRANSIT_II_NIGHT_DAZZLING      ((uint32_t)0x004A8000)/*���ڹ��ɶΣ�ҹ��ʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_II_NIGHT_BRIGHT        ((uint32_t)0x004A9000)/*���ڹ��ɶΣ�ҹ��ʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_II_NIGHT_VISIBLE       ((uint32_t)0x004AA000)/*���ڹ��ɶΣ�ҹ��ʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_II_NIGHT_UNSEEN        ((uint32_t)0x004AB000)/*���ڹ��ɶΣ�ҹ��ʱ��Σ��޹������*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_DAZZLING   ((uint32_t)0x004AC000)/*���ڹ��ɶΣ���ҹʱ��Σ�ǿ�������*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_BRIGHT     ((uint32_t)0x004AD000)/*���ڹ��ɶΣ���ҹʱ��Σ��й������*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_VISIBLE    ((uint32_t)0x004AE000)/*���ڹ��ɶΣ���ҹʱ��Σ����������*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_UNSEEN     ((uint32_t)0x004AF000)/*���ڹ��ɶΣ���ҹʱ��Σ��޹������*/ 

/*���ڶβ��Դ洢*/
#define STRATEGY_EXPORT_ONOFF_DAZZLING          ((uint32_t)0x004C0000)/*���ڶΣ����ص�ʱ��Σ�ǿ�������*/ 
#define STRATEGY_EXPORT_ONOFF_BRIGHT            ((uint32_t)0x004C1000)/*���ڶΣ����ص�ʱ��Σ��й������*/ 
#define STRATEGY_EXPORT_ONOFF_VISIBLE           ((uint32_t)0x004C2000)/*���ڶΣ����ص�ʱ��Σ����������*/ 
#define STRATEGY_EXPORT_ONOFF_UNSEEN            ((uint32_t)0x004C3000)/*���ڶΣ����ص�ʱ��Σ��޹������*/ 
#define STRATEGY_EXPORT_DAY_DAZZLING            ((uint32_t)0x004C4000)/*���ڶΣ�����ʱ��Σ�ǿ�������*/ 
#define STRATEGY_EXPORT_DAY_BRIGHT              ((uint32_t)0x004C5000)/*���ڶΣ�����ʱ��Σ��й������*/ 
#define STRATEGY_EXPORT_DAY_VISIBLE             ((uint32_t)0x004C6000)/*���ڶΣ�����ʱ��Σ����������*/ 
#define STRATEGY_EXPORT_DAY_UNSEEN              ((uint32_t)0x004C7000)/*���ڶΣ�����ʱ��Σ��޹������*/ 
#define STRATEGY_EXPORT_NIGHT_DAZZLING          ((uint32_t)0x004C8000)/*���ڶΣ�ҹ��ʱ��Σ�ǿ�������*/ 
#define STRATEGY_EXPORT_NIGHT_BRIGHT            ((uint32_t)0x004C9000)/*���ڶΣ�ҹ��ʱ��Σ��й������*/ 
#define STRATEGY_EXPORT_NIGHT_VISIBLE           ((uint32_t)0x004CA000)/*���ڶΣ�ҹ��ʱ��Σ����������*/ 
#define STRATEGY_EXPORT_NIGHT_UNSEEN            ((uint32_t)0x004CB000)/*���ڶΣ�ҹ��ʱ��Σ��޹������*/ 
#define STRATEGY_EXPORT_MIDNIGHT_DAZZLING       ((uint32_t)0x004CC000)/*���ڶΣ���ҹʱ��Σ�ǿ�������*/ 
#define STRATEGY_EXPORT_MIDNIGHT_BRIGHT         ((uint32_t)0x004CD000)/*���ڶΣ���ҹʱ��Σ��й������*/ 
#define STRATEGY_EXPORT_MIDNIGHT_VISIBLE        ((uint32_t)0x004CE000)/*���ڶΣ���ҹʱ��Σ����������*/ 
#define STRATEGY_EXPORT_MIDNIGHT_UNSEEN         ((uint32_t)0x004CF000)/*���ڶΣ���ҹʱ��Σ��޹������*/ 

#define GATEWAY_RUN_DAY                         ((uint32_t)0x004E0000)/*������������*/


void NorFlashInit(void);
void NorFlashWrite(uint32_t flash, const short *ram, int len);
void NorFlashEraseParam(uint32_t flash);
void NorFlashRead(uint32_t flash, short *ram, int len);
void NorFlashEraseChip(void);
void NorFlashEraseBlock(uint32_t block) ;

bool NorFlashMutexLock(uint32_t time);
void NorFlashMutexUnlock(void);


#endif
