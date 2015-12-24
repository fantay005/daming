#include "stm32f10x_flash.h"
#include "inside_flash.h"


#define STM32_FLASH_SIZE 	512 	 		//��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH����ʼ��ַ


#if STM32_FLASH_SIZE < 256
#define STM_SECTOR_SIZE 1024 //�ֽ�
#else 
#define STM_SECTOR_SIZE	2048
#endif		 

uint16_t STMFLASH_BUF[STM_SECTOR_SIZE / 2];//�����2K�ֽ�


void STMFLASH_Write_NoCheck(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite) { 			 		 
	uint16_t i;
	
	for(i = 0; i < NumToWrite; i++){
		FLASH_ProgramHalfWord(WriteAddr, pBuffer[i]);
	  WriteAddr += 2;//��ַ����2.
	}  
} 

/*ÿ�ζ�ȡһ���ֽ�*/

uint8_t FLASH_ReadByte(uint32_t faddr)
{
	return *(vu8*)faddr; 
}

void STMFLASH_Visit(uint32_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead) {
	uint16_t i;
	
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i] = FLASH_ReadByte(ReadAddr);//��ȡ2���ֽ�.
		ReadAddr++;//ƫ��2���ֽ�.	
	}
}

/*ÿ�ζ�ȡһ�����֣������ֽڣ�*/

uint16_t FLASH_ReadHalfWord(uint32_t faddr)
{
	return *(vu16*)faddr; 
}

void STMFLASH_Read(uint32_t ReadAddr, uint16_t *pBuffer, uint16_t NumToRead) {
	uint16_t i;
	
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i] = FLASH_ReadHalfWord(ReadAddr);//��ȡ2���ֽ�.
		ReadAddr+=2;//ƫ��2���ֽ�.	
	}
}

void STMFLASH_Write(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite){
	uint32_t secpos;	   //������ַ
	uint16_t secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
	uint16_t secremain; //������ʣ���ַ(16λ�ּ���)	   
 	uint16_t i;    
	uint32_t offaddr;   //ȥ��0X08000000��ĵ�ַ
	
	if(WriteAddr < STM32_FLASH_BASE || (WriteAddr >= (STM32_FLASH_BASE + 1024 * STM32_FLASH_SIZE))){
		return;//�Ƿ���ַ
	}
	FLASH_Unlock();						//����
	FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
	offaddr = WriteAddr - STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
	secpos = offaddr / STM_SECTOR_SIZE;			//������ַ
	secoff = (offaddr % STM_SECTOR_SIZE) / 2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
	secremain = STM_SECTOR_SIZE / 2- secoff;		//����ʣ��ռ��С   
	if(NumToWrite <= secremain) {
		secremain = NumToWrite;//�����ڸ�������Χ
	}
	while(1) {	
		STMFLASH_Read(secpos * STM_SECTOR_SIZE + STM32_FLASH_BASE, STMFLASH_BUF, STM_SECTOR_SIZE / 2);//������������������
		
		for(i = 0; i < secremain; i++)//У������
		{
			if(STMFLASH_BUF[secoff + i] != 0XFFFF) {
				break;//��Ҫ����  	  
			}
		}
		
		if(i < secremain)//��Ҫ����
		{
			FLASH_ErasePage(secpos * STM_SECTOR_SIZE + STM32_FLASH_BASE);//�����������
			for(i = 0; i < secremain; i++)//����
			{
				STMFLASH_BUF[i + secoff] = pBuffer[i];	  
			}
			STMFLASH_Write_NoCheck(secpos * STM_SECTOR_SIZE + STM32_FLASH_BASE, STMFLASH_BUF, STM_SECTOR_SIZE / 2);//д����������  
		} else {
			STMFLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);//д�Ѿ������˵�,ֱ��д������ʣ������. 	
		}			
		
		if(NumToWrite <= secremain){
			break;//д�������
		} else {//д��δ����
			secpos++;				//������ַ��1
			secoff = 0;				//ƫ��λ��Ϊ0 	 
		 	pBuffer += secremain;  	//ָ��ƫ��
			WriteAddr += secremain;	//д��ַƫ��	   
		 	NumToWrite -= secremain;	//�ֽ�(16λ)���ݼ�
			
			if(NumToWrite > (STM_SECTOR_SIZE / 2)){
				secremain = STM_SECTOR_SIZE / 2;//��һ����������д����
			} else {
				secremain = NumToWrite;//��һ����������д����
			}
		}	 
	}
	FLASH_Lock();//����
}
