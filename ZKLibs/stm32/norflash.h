#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include <stdbool.h>
#include <stdint.h>
#include "fsmc_nor.h"

/********************************************************************************************************
* NOR_FLASH地址映射表
********************************************************************************************************/
//NOR_FLASH
#define NORFLASH_SECTOR_SIZE   				          ((uint32_t)0x00001000)/*一个扇区的大小*/  
#define NORFLASH_BLOCK_SIZE                     ((uint32_t)0x00010000)/*一块的大小*/
#define NORFLASH_MANAGEM_BASE  				          ((uint32_t)0x00001000)/*网关参数-网关身份标识、经纬度、ZIGBEE频点、自动上传数据时间间隔*/
#define NORFLASH_BALLAST_NUM   				          ((uint32_t)0x00002000)/*镇流器数目*/
#define NORFLASH_CHIP_ERASE                     ((uint32_t)0x00005000)/*D*/ 
#define NORFLASH_STRATEGY_ADDR                  ((uint32_t)0x00006000)/*隧道网关下镇流器统一策略放置地址*/


#define NORFLASH_LIGHT_NUMBER                   ((uint32_t)0x00009000)/*第一个short类型数据位下载的灯参数量，即控制的灯总量，第二个short类型数据为最大的zigbee地址*/
#define NORFLAH_ERASE_BLOCK_BASE                ((uint32_t)0x00010000)/*擦除块起始地址*/
#define NORFLASH_BALLAST_BASE  				          ((uint32_t)0x00018000)/*Zigbee1 镇流器参数基址*/
#define NORFLASH_BSN_PARAM_BASE                 ((uint32_t)0x00218000)/*Zigbee2 镇流器参数基址*/
#define NORFLASH_MANAGEM_ADDR                   ((uint32_t)0x00400000)/*网关地址、服务器IP地址、端口号*/
#define NORFLASH_MANAGEM_TIMEOFFSET 		        ((uint32_t)0x00401000)/*网关参数-开灯偏移、关灯偏移*/
#define NORFLASH_MANAGEM_WARNING   		          ((uint32_t)0x00402000)/*网关参数-告警*/
#define NORFLASH_RESET_TIME           		    	((uint32_t)0x00404000)/*重启时间*/
#define NORFLASH_RESET_COUNT                    ((uint32_t)0x00405000)/*重启次数*/
#define NORFLASH_ELEC_UPDATA_TIME               ((uint32_t)0x00406000)/*电量上传时间*/
#define NORFLASH_PARAM_OFFSET   				        ((uint32_t)0x00001000)
#define NORFLASH_STRATEGY_OFFSET        		    ((uint32_t)0x00001000)

/*隧道内网关参数及策略在扇区内偏移*/
#define PARAM_LUX_DOG_OFFSET                    ((uint32_t)0x00417000)/*光照度区域划分点参数*/              
#define PARAM_TIME_DOG_OFFSET                   ((uint32_t)0x00418000)/*时间区域划分点参数*/

#define SECTION_SPACE_SIZE                      ((uint32_t)0x00020000)/*段存储相隔空间大小*/

/*引导段策略存储*/
#define STRATEGY_GUIDE_ONOFF_DAZZLING           ((uint32_t)0x00420000)/*引导段，开关灯时间段，强光域策略*/ 
#define STRATEGY_GUIDE_ONOFF_BRIGHT             ((uint32_t)0x00421000)/*引导段，开关灯时间段，中光域策略*/ 
#define STRATEGY_GUIDE_ONOFF_VISIBLE            ((uint32_t)0x00422000)/*引导段，开关灯时间段，弱光域策略*/ 
#define STRATEGY_GUIDE_ONOFF_UNSEEN             ((uint32_t)0x00423000)/*引导段，开关灯时间段，无光域策略*/ 
#define STRATEGY_GUIDE_DAY_DAZZLING             ((uint32_t)0x00424000)/*引导段，白天时间段，强光域策略*/ 
#define STRATEGY_GUIDE_DAY_BRIGHT               ((uint32_t)0x00425000)/*引导段，白天时间段，中光域策略*/ 
#define STRATEGY_GUIDE_DAY_VISIBLE              ((uint32_t)0x00426000)/*引导段，白天时间段，弱光域策略*/ 
#define STRATEGY_GUIDE_DAY_UNSEEN               ((uint32_t)0x00427000)/*引导段，白天时间段，无光域策略*/ 
#define STRATEGY_GUIDE_NIGHT_DAZZLING           ((uint32_t)0x00428000)/*引导段，夜晚时间段，强光域策略*/ 
#define STRATEGY_GUIDE_NIGHT_BRIGHT             ((uint32_t)0x00429000)/*引导段，夜晚时间段，中光域策略*/ 
#define STRATEGY_GUIDE_NIGHT_VISIBLE            ((uint32_t)0x0042A000)/*引导段，夜晚时间段，弱光域策略*/ 
#define STRATEGY_GUIDE_NIGHT_UNSEEN             ((uint32_t)0x0042B000)/*引导段，夜晚时间段，无光域策略*/ 
#define STRATEGY_GUIDE_MIDNIGHT_DAZZLING        ((uint32_t)0x0042C000)/*引导段，深夜时间段，强光域策略*/ 
#define STRATEGY_GUIDE_MIDNIGHT_BRIGHT          ((uint32_t)0x0042D000)/*引导段，深夜时间段，中光域策略*/ 
#define STRATEGY_GUIDE_MIDNIGHT_VISIBLE         ((uint32_t)0x0042E000)/*引导段，深夜时间段，弱光域策略*/ 
#define STRATEGY_GUIDE_MIDNIGHT_UNSEEN          ((uint32_t)0x0042F000)/*引导段，深夜时间段，无光域策略*/ 

/*入口段策略存储*/
#define STRATEGY_ENTRY_ONOFF_DAZZLING           ((uint32_t)0x00440000)/*入口段，开关灯时间段，强光域策略*/ 
#define STRATEGY_ENTRY_ONOFF_BRIGHT             ((uint32_t)0x00441000)/*入口段，开关灯时间段，中光域策略*/ 
#define STRATEGY_ENTRY_ONOFF_VISIBLE            ((uint32_t)0x00442000)/*入口段，开关灯时间段，弱光域策略*/ 
#define STRATEGY_ENTRY_ONOFF_UNSEEN             ((uint32_t)0x00443000)/*入口段，开关灯时间段，无光域策略*/ 
#define STRATEGY_ENTRY_DAY_DAZZLING             ((uint32_t)0x00444000)/*入口段，白天时间段，强光域策略*/ 
#define STRATEGY_ENTRY_DAY_BRIGHT               ((uint32_t)0x00445000)/*入口段，白天时间段，中光域策略*/ 
#define STRATEGY_ENTRY_DAY_VISIBLE              ((uint32_t)0x00446000)/*入口段，白天时间段，弱光域策略*/ 
#define STRATEGY_ENTRY_DAY_UNSEEN               ((uint32_t)0x00447000)/*入口段，白天时间段，无光域策略*/ 
#define STRATEGY_ENTRY_NIGHT_DAZZLING           ((uint32_t)0x00448000)/*入口段，夜晚时间段，强光域策略*/ 
#define STRATEGY_ENTRY_NIGHT_BRIGHT             ((uint32_t)0x00449000)/*入口段，夜晚时间段，中光域策略*/ 
#define STRATEGY_ENTRY_NIGHT_VISIBLE            ((uint32_t)0x0044A000)/*入口段，夜晚时间段，弱光域策略*/ 
#define STRATEGY_ENTRY_NIGHT_UNSEEN             ((uint32_t)0x0044B000)/*入口段，夜晚时间段，无光域策略*/ 
#define STRATEGY_ENTRY_MIDNIGHT_DAZZLING        ((uint32_t)0x0044C000)/*入口段，深夜时间段，强光域策略*/ 
#define STRATEGY_ENTRY_MIDNIGHT_BRIGHT          ((uint32_t)0x0044D000)/*入口段，深夜时间段，中光域策略*/ 
#define STRATEGY_ENTRY_MIDNIGHT_VISIBLE         ((uint32_t)0x0044E000)/*入口段，深夜时间段，弱光域策略*/ 
#define STRATEGY_ENTRY_MIDNIGHT_UNSEEN          ((uint32_t)0x0044F000)/*入口段，深夜时间段，无光域策略*/ 

/*入口过渡段策略存储*/
#define STRATEGY_TRANSIT_I_ONOFF_DAZZLING       ((uint32_t)0x00460000)/*入口过渡段，开关灯时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_I_ONOFF_BRIGHT         ((uint32_t)0x00461000)/*入口过渡段，开关灯时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_I_ONOFF_VISIBLE        ((uint32_t)0x00462000)/*入口过渡段，开关灯时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_I_ONOFF_UNSEEN         ((uint32_t)0x00463000)/*入口过渡段，开关灯时间段，无光域策略*/ 
#define STRATEGY_TRANSIT_I_DAY_DAZZLING         ((uint32_t)0x00464000)/*入口过渡段，白天时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_I_DAY_BRIGHT           ((uint32_t)0x00465000)/*入口过渡段，白天时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_I_DAY_VISIBLE          ((uint32_t)0x00466000)/*入口过渡段，白天时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_I_DAY_UNSEEN           ((uint32_t)0x00467000)/*入口过渡段，白天时间段，无光域策略*/ 
#define STRATEGY_TRANSIT_I__NIGHT_DAZZLING      ((uint32_t)0x00468000)/*入口过渡段，夜晚时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_I__NIGHT_BRIGHT        ((uint32_t)0x00469000)/*入口过渡段，夜晚时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_I__NIGHT_VISIBLE       ((uint32_t)0x0046A000)/*入口过渡段，夜晚时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_I__NIGHT_UNSEEN        ((uint32_t)0x0046B000)/*入口过渡段，夜晚时间段，无光域策略*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_DAZZLING   ((uint32_t)0x0046C000)/*入口过渡段，深夜时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_BRIGHT     ((uint32_t)0x0046D000)/*入口过渡段，深夜时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_VISIBLE    ((uint32_t)0x0046E000)/*入口过渡段，深夜时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_I__MIDNIGHT_UNSEEN     ((uint32_t)0x0046F000)/*入口过渡段，深夜时间段，无光域策略*/ 

/*中间段策略存储*/
#define STRATEGY_MIDDLE_ONOFF_DAZZLING          ((uint32_t)0x00480000)/*中间段，开关灯时间段，强光域策略*/ 
#define STRATEGY_MIDDLE_ONOFF_BRIGHT            ((uint32_t)0x00481000)/*中间段，开关灯时间段，中光域策略*/ 
#define STRATEGY_MIDDLE_ONOFF_VISIBLE           ((uint32_t)0x00482000)/*中间段，开关灯时间段，弱光域策略*/ 
#define STRATEGY_MIDDLE_ONOFF_UNSEEN            ((uint32_t)0x00483000)/*中间段，开关灯时间段，无光域策略*/ 
#define STRATEGY_MIDDLE_DAY_DAZZLING            ((uint32_t)0x00484000)/*中间段，白天时间段，强光域策略*/ 
#define STRATEGY_MIDDLE_DAY_BRIGHT              ((uint32_t)0x00485000)/*中间段，白天时间段，中光域策略*/ 
#define STRATEGY_MIDDLE_DAY_VISIBLE             ((uint32_t)0x00486000)/*中间段，白天时间段，弱光域策略*/ 
#define STRATEGY_MIDDLE_DAY_UNSEEN              ((uint32_t)0x00487000)/*中间段，白天时间段，无光域策略*/ 
#define STRATEGY_MIDDLE_NIGHT_DAZZLING          ((uint32_t)0x00488000)/*中间段，夜晚时间段，强光域策略*/ 
#define STRATEGY_MIDDLE_NIGHT_BRIGHT            ((uint32_t)0x00489000)/*中间段，夜晚时间段，中光域策略*/ 
#define STRATEGY_MIDDLE_NIGHT_VISIBLE           ((uint32_t)0x0048A000)/*中间段，夜晚时间段，弱光域策略*/ 
#define STRATEGY_MIDDLE_NIGHT_UNSEEN            ((uint32_t)0x0048B000)/*中间段，夜晚时间段，无光域策略*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_DAZZLING       ((uint32_t)0x0048C000)/*中间段，深夜时间段，强光域策略*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_BRIGHT         ((uint32_t)0x0048D000)/*中间段，深夜时间段，中光域策略*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_VISIBLE        ((uint32_t)0x0048E000)/*中间段，深夜时间段，弱光域策略*/ 
#define STRATEGY_MIDDLE_MIDNIGHT_UNSEEN         ((uint32_t)0x0048F000)/*中间段，深夜时间段，无光域策略*/ 

/*出口过渡段策略存储*/
#define STRATEGY_TRANSIT_II_ONOFF_DAZZLING      ((uint32_t)0x004A0000)/*出口过渡段，开关灯时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_II_ONOFF_BRIGHT        ((uint32_t)0x004A1000)/*出口过渡段，开关灯时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_II_ONOFF_VISIBLE       ((uint32_t)0x004A2000)/*出口过渡段，开关灯时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_II_ONOFF_UNSEEN        ((uint32_t)0x004A3000)/*出口过渡段，开关灯时间段，无光域策略*/ 
#define STRATEGY_TRANSIT_II_DAY_DAZZLING        ((uint32_t)0x004A4000)/*出口过渡段，白天时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_II_DAY_BRIGHT          ((uint32_t)0x004A5000)/*出口过渡段，白天时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_II_DAY_VISIBLE         ((uint32_t)0x004A6000)/*出口过渡段，白天时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_II_DAY_UNSEEN          ((uint32_t)0x004A7000)/*出口过渡段，白天时间段，无光域策略*/ 
#define STRATEGY_TRANSIT_II_NIGHT_DAZZLING      ((uint32_t)0x004A8000)/*出口过渡段，夜晚时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_II_NIGHT_BRIGHT        ((uint32_t)0x004A9000)/*出口过渡段，夜晚时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_II_NIGHT_VISIBLE       ((uint32_t)0x004AA000)/*出口过渡段，夜晚时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_II_NIGHT_UNSEEN        ((uint32_t)0x004AB000)/*出口过渡段，夜晚时间段，无光域策略*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_DAZZLING   ((uint32_t)0x004AC000)/*出口过渡段，深夜时间段，强光域策略*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_BRIGHT     ((uint32_t)0x004AD000)/*出口过渡段，深夜时间段，中光域策略*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_VISIBLE    ((uint32_t)0x004AE000)/*出口过渡段，深夜时间段，弱光域策略*/ 
#define STRATEGY_TRANSIT_II_MIDNIGHT_UNSEEN     ((uint32_t)0x004AF000)/*出口过渡段，深夜时间段，无光域策略*/ 

/*出口段策略存储*/
#define STRATEGY_EXPORT_ONOFF_DAZZLING          ((uint32_t)0x004C0000)/*出口段，开关灯时间段，强光域策略*/ 
#define STRATEGY_EXPORT_ONOFF_BRIGHT            ((uint32_t)0x004C1000)/*出口段，开关灯时间段，中光域策略*/ 
#define STRATEGY_EXPORT_ONOFF_VISIBLE           ((uint32_t)0x004C2000)/*出口段，开关灯时间段，弱光域策略*/ 
#define STRATEGY_EXPORT_ONOFF_UNSEEN            ((uint32_t)0x004C3000)/*出口段，开关灯时间段，无光域策略*/ 
#define STRATEGY_EXPORT_DAY_DAZZLING            ((uint32_t)0x004C4000)/*出口段，白天时间段，强光域策略*/ 
#define STRATEGY_EXPORT_DAY_BRIGHT              ((uint32_t)0x004C5000)/*出口段，白天时间段，中光域策略*/ 
#define STRATEGY_EXPORT_DAY_VISIBLE             ((uint32_t)0x004C6000)/*出口段，白天时间段，弱光域策略*/ 
#define STRATEGY_EXPORT_DAY_UNSEEN              ((uint32_t)0x004C7000)/*出口段，白天时间段，无光域策略*/ 
#define STRATEGY_EXPORT_NIGHT_DAZZLING          ((uint32_t)0x004C8000)/*出口段，夜晚时间段，强光域策略*/ 
#define STRATEGY_EXPORT_NIGHT_BRIGHT            ((uint32_t)0x004C9000)/*出口段，夜晚时间段，中光域策略*/ 
#define STRATEGY_EXPORT_NIGHT_VISIBLE           ((uint32_t)0x004CA000)/*出口段，夜晚时间段，弱光域策略*/ 
#define STRATEGY_EXPORT_NIGHT_UNSEEN            ((uint32_t)0x004CB000)/*出口段，夜晚时间段，无光域策略*/ 
#define STRATEGY_EXPORT_MIDNIGHT_DAZZLING       ((uint32_t)0x004CC000)/*出口段，深夜时间段，强光域策略*/ 
#define STRATEGY_EXPORT_MIDNIGHT_BRIGHT         ((uint32_t)0x004CD000)/*出口段，深夜时间段，中光域策略*/ 
#define STRATEGY_EXPORT_MIDNIGHT_VISIBLE        ((uint32_t)0x004CE000)/*出口段，深夜时间段，弱光域策略*/ 
#define STRATEGY_EXPORT_MIDNIGHT_UNSEEN         ((uint32_t)0x004CF000)/*出口段，深夜时间段，无光域策略*/ 

#define GATEWAY_RUN_DAY                         ((uint32_t)0x004E0000)/*网关运行天数*/


void NorFlashInit(void);
void NorFlashWrite(uint32_t flash, const short *ram, int len);
void NorFlashEraseParam(uint32_t flash);
void NorFlashRead(uint32_t flash, short *ram, int len);
void NorFlashEraseChip(void);
void NorFlashEraseBlock(uint32_t block) ;

bool NorFlashMutexLock(uint32_t time);
void NorFlashMutexUnlock(void);


#endif
