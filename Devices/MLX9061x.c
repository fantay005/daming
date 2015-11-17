#include "mlx9061x.h"

static
unsigned char __PEC_calculation(unsigned char pec[])
{
	unsigned char 	crc[6];
	unsigned char	BitPosition=47;
	unsigned char	shift;
	unsigned char	i;
	unsigned char	j;
	unsigned char	temp;

	do{
		crc[5]=0;				/* Load CRC value 0x000000000107 */
		crc[4]=0;
		crc[3]=0;
		crc[2]=0;
		crc[1]=0x01;
		crc[0]=0x07;
		BitPosition=47;			/* Set maximum bit position at 47 */
		shift=0;
				
		//Find first 1 in the transmited message
		i=5;					/* Set highest index */
		j=0;
		while((pec[i]&(0x80>>j))==0 && i>0){
			BitPosition--;
			if(j<7){
				j++;
			}
			else{
				j=0x00;
				i--;
			}
		}/*End of while */
		
		shift=BitPosition-8;	/*Get shift value for crc value*/
		
		
		//Shift crc value 
		while(shift){
			for(i=5; i<0xFF; i--){
				if((crc[i-1]&0x80) && (i>0)){
					temp=1;
				}
				else{
					temp=0;
				}
				crc[i]<<=1;
				crc[i]+=temp;
			}/*End of for*/
			shift--;
		}/*End of while*/
		
		
		//Exclusive OR between pec and crc		
		for(i=0; i<=5; i++){
			pec[i] ^=crc[i];
		}/*End of for*/
	}while(BitPosition>8);/*End of do-while*/
	
	return pec[0];
}/*End of PEC_calculation*/

uint16_t MLX9061x_memRead(const SoftI2C_t *i2c, uint8_t sAddr, uint8_t mAddr)
{
	unsigned int  data;				// Data storage (DataH:DataL)
	unsigned char Pec;				// PEC byte storage
	unsigned char DataL = 0;			// Low data byte storage
	unsigned char DataH = 0;			// High data byte storage
	unsigned char arr[6];			// Buffer for the sent bytes
	unsigned char PecReg;			// Calculated PEC byte storage
	unsigned char ErrorCounter;		// Defines the number of the attempts for communication with MLX90614
	
	ErrorCounter=0x00;				// Initialising of ErrorCounter
	
	while(1){
		SoftI2C_Stop(i2c);
		--ErrorCounter;				//Pre-decrement ErrorCounter
		if(!ErrorCounter){			//ErrorCounter=0?
			break;					//Yes,go out from do-while{}
		}
		SoftI2C_Start(i2c);				//Start condition

		if(!SoftI2C_Write(i2c, sAddr)){
			continue;
		}
		
		if(!SoftI2C_Write(i2c, mAddr)){
			continue;
		}

		SoftI2C_Start(i2c);	
		
		if(!SoftI2C_Write(i2c, sAddr|0x01)){
			continue;
		}
					
		DataL=SoftI2C_Read(i2c,1);			//Read low data,master must send ACK
		DataH=SoftI2C_Read(i2c,1); 		//Read high data,master must send ACK

		Pec=SoftI2C_Read(i2c,0);			//Read PEC byte, master must send NACK

		
		arr[5]=sAddr;		//
		arr[4]=mAddr;				//
		arr[3]=sAddr;		//Load array arr 
		arr[2]=DataL;				//
		arr[1]=DataH;				//
		arr[0]=0;					//
		PecReg=__PEC_calculation(arr);//Calculate CRC
	    if((PecReg != Pec)) break;  
	}		//If received and calculated CRC are equal go out from do-while{}
	SoftI2C_Stop(i2c);					//Stop condition
	return ((DataH<<8)|DataL);
}


void MLX9061x_Start(void)
{
	MLX90614_Start();
	MLX90615_Start();
}

void MLX9061x_Stop(void)
{
	MLX90614_Stop();
	MLX90615_Stop();
}

void MLX9061x_Config(void){
	MLX90614_Config();
	MLX90615_Config();
}

