/****************************************************/
/*      Function        :       S3G I2C Serial Port Control                             */
/*      Author  :       Janice Yi                                                               */
/*      Date            :       Feb 16th, 2011                                          */
/*      Rev.            :       1.0.0.1                                                         */
/*  Compiler    :       Watcom(DOS/4GW)                                 */
/****************************************************/

#include "i2c.h"

#include <bios.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>


// Serial Port Enable
#define S3_SPC_ENABLE   BIT4
#define S3_SCL_ENABLE   BIT4
#define S3_SDA_ENABLE   BIT4
#define S3_SCL_READ     BIT2
#define S3_SDA_READ     BIT3
#define S3_SCL_WRITE    BIT0
#define S3_SDA_WRITE    BIT1
#define PCI_VENDORID_S3     0x5333
#define PCI_DEVICEID1_D4        0x9060
#define PCI_DEVICEID2_D4        0x9040
#define PCI_VENDORID_VIA        0x1D17
#define PCI_DEVICEID3_UMA       0x6120
#define PCI_DEVICEID_CHX001     0x3a00
#define TRUE 1
#define FALSE 0


void SerialPort_RegWrite(unsigned index, unsigned value);
unsigned SerialPort_RegRead(unsigned index);
void i2c_start(void);
void i2c_stop(void);
int i2c_read();
void i2c_write (unsigned addr);
int i2c_ack_read(void);
int i2c_nack_write();
int i2c_ack_write();
void i2c_delay(unsigned i);
unsigned int ReadPciCfgDword(unsigned char bus, unsigned char dev, unsigned char func, unsigned char reg);
unsigned short ReadPciCfgWord(unsigned char bus, unsigned char dev, unsigned char func, unsigned char reg);
void CrbMMIOWrite(unsigned int mmiobase, unsigned int index, unsigned int value);
unsigned int CrbMMIORead(unsigned int mmiobase, unsigned int index);
void Write8(unsigned int mmiobase, unsigned int index, unsigned int value);
unsigned char Read8(unsigned int mmiobase, unsigned int index);



unsigned char SCL_WL = 0;    
unsigned char SCL_WH = 0;    
unsigned char SDA_WL = 0;    
unsigned char SDA_WH = 0; 
unsigned char SDA_BIT = 0;
unsigned char SDA_SHIFT = 0;
unsigned char SCL_BIT = 0;
unsigned char SCL_SHIFT = 0;
unsigned char SDA_R = 0;

unsigned char slave_addr;
unsigned char serial;

//unsigned int g_mmiobase = 0;
typedef unsigned char      CBIOS_U8,    *PCBIOS_U8;
typedef unsigned int       CBIOS_U32,   *PCBIOS_U32;
typedef unsigned short     CBIOS_U16,   *PCBIOS_U16;

unsigned int GetMMIOBase()
{
        unsigned char  bus, dev;
        unsigned int MMIOBaseAddress;
        unsigned short vendorID;
        unsigned short deviceID;
        
        for ( bus = 0; bus < 255; bus++)
        {
                for ( dev = 0; dev < 32; dev++)
                {
                        vendorID = ReadPciCfgWord(bus, dev, 0, 0x00);
                        
                        if (vendorID != PCI_VENDORID_VIA)
                        {
                                continue;
                        }

                        deviceID = ReadPciCfgWord(bus, dev, 0, 0x02);

                        switch (deviceID&0xFFF0)
                        {
                                //case PCI_VENDORID_S3:
                                //case PCI_DEVICEID1_D4:
                                //case PCI_DEVICEID2_D4:
                                //case PCI_VENDORID_VIA:
                                case PCI_DEVICEID3_UMA:
                                case PCI_DEVICEID_CHX001:
                                        MMIOBaseAddress = ReadPciCfgDword(bus, dev, 0, 0x10);
                                        MMIOBaseAddress &= 0xFFFF0000;
                                        return MMIOBaseAddress;
                                        break;
                                default:
                                        break;
                        }
                }
        }
        return -1;
}
unsigned int ReadPciCfgDword(unsigned char bus, unsigned char dev, unsigned char func, unsigned char reg)
{
        unsigned int v, flag = 0;

        _asm pushfd;
        _asm pop flag;
        _asm cli;
        outpd(0xCF8, (0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC)));
        v = inpd(0xCFC);
        _asm push flag;
        _asm popfd;

        return v;
}
unsigned short ReadPciCfgWord(unsigned char bus, unsigned char dev, unsigned char func, unsigned char reg)
{
        unsigned short v;
        unsigned int flag = 0;

        _asm pushfd;
        _asm pop flag;
        _asm cli;
        outpd(0xCF8, (0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(reg&0xFC)));
        v = inpw(0xCFC+(reg&3));
        _asm push flag;
        _asm popfd;

        return v;
}
void CrbMMIOWrite(unsigned int mmiobase, unsigned int index, unsigned int value)
{
        Write8((mmiobase+0x8900), index, value);

}

unsigned int CrbMMIORead(unsigned int mmiobase, unsigned int index)
{
        unsigned int value;

        value = (unsigned int)Read8(mmiobase+0x8900, index);

        return value;
}
//S3G register control
//Access IO through MMIO index
void CrMMIOWrite(unsigned int mmiobase, unsigned int index,unsigned int value)
{
        Write8((mmiobase+0x8800), index, value);
}

unsigned int CrMMIORead(unsigned int mmiobase, unsigned int index)
{
        unsigned int value;

        value = (unsigned int)Read8(mmiobase+0x8800, index);

        return value;
}
void Write8(unsigned int mmiobase, unsigned int index, unsigned int value)
{
        *(unsigned char*)(mmiobase + index) = value;
}

unsigned char Read8(unsigned int mmiobase, unsigned int index)
{
    return *((unsigned char*)(mmiobase+index));
}



//S3G I2C Bus Initiate
void SerialPort_Init(unsigned char serialno, unsigned char slaveaddr)
{
        serial = serialno;      
        slave_addr = slaveaddr;
        SCL_WL = S3_SCL_ENABLE|0;                               //8'b0001 0000; enable Serial Port 1
        SCL_WH = S3_SCL_ENABLE|S3_SCL_WRITE;    //8'b0001 0001; enable SP1; SPCLK1 is tri-stated
        SDA_WL = S3_SDA_ENABLE|0;                               //8'b0001 0000; enable Serial Port 1
        SDA_WH = S3_SDA_ENABLE|S3_SDA_WRITE;    //8'b0001 0010; enable SP1; SPDAT1 is tri-stated
        SDA_R = S3_SDA_ENABLE|S3_SDA_WRITE;     //8'b0001 0010; enable SP1; SPDAT1 is tri-stated
        SDA_BIT = S3_SDA_READ;                                  //8'b0000 1000;
        SDA_SHIFT = 3;
        SCL_BIT = S3_SCL_READ;                                  //8'b0000 0100;
        SCL_SHIFT = 2;
}

void SerialPort_RegWrite(unsigned index, unsigned value)
{

        if ((serial == S3G_SERIAL_CRT) || (serial == S3G_SERIAL_HDMI2) || (serial == S3G_SERIAL_HDMI3))
                CrbMMIOWrite(g_mmiobase, index, value);         
        else if ((serial == S3G_SERIAL_HDMI0) || (serial == S3G_SERIAL_HDMI1))
                CrMMIOWrite(g_mmiobase, index, value);  
        else 
        {
                printf("Wrong I2C serial port number 0x%2.2x\n", serial);              
        }
        
}

unsigned SerialPort_RegRead(unsigned index)
{
        unsigned value;

        if ((serial == S3G_SERIAL_CRT) || (serial == S3G_SERIAL_HDMI2) || (serial == S3G_SERIAL_HDMI3))
                value = CrbMMIORead(g_mmiobase, index);         
        else if ((serial == S3G_SERIAL_HDMI0) || (serial == S3G_SERIAL_HDMI1))
                value = CrMMIORead(g_mmiobase, index);  
        else 
        {
                printf("Wrong I2C serial port number 0x%2.2x\n", serial);              
        }

        return(value);
}

void i2c_start()        //Generating the I2C's START signal which is a high-to-low transition of SDA with SCL high
{

        SerialPort_RegWrite(serial, SCL_WH|SDA_WH);     //CRA0=0x13     
        i2c_delay(DELAYTIME);
        SerialPort_RegWrite(serial, SCL_WH|SDA_WL);     //CRA0=0x11        
        i2c_delay(DELAYTIME);
        SerialPort_RegWrite(serial, SCL_WL|SDA_WL);             //CRA0=0x10      
        i2c_delay(DELAYTIME);

}

void i2c_stop()
{
        int j;

        SerialPort_RegWrite(serial, SCL_WL|SDA_WL); 
        i2c_delay(DELAYTIME);
        j = 0;
        while(j++ < RETRYTIMES)
        {
                SerialPort_RegWrite(serial, SCL_WH|SDA_WL); 
                i2c_delay(DELAYTIME);
                if(((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)&&(!((SerialPort_RegRead(serial)>>SDA_SHIFT)&0x01)))
                        break;
        }        
        j = 0;
        while(j++ < RETRYTIMES)
        {
                SerialPort_RegWrite(serial, SCL_WH|SDA_WH); 
                i2c_delay(DELAYTIME);
                if(((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)&&((SerialPort_RegRead(serial)>>SDA_SHIFT)&0x01))
                        break;
        }                
}

int i2c_read()
{
        int i, data=0;
        int j;
        int maxloop = RETRYTIMES;
        for (i=0; i<8; i++)
        {
                SerialPort_RegWrite (serial, SCL_WL|SDA_R); 
                i2c_delay(DELAYTIME/2);

                j = 0;
                while(j++ < maxloop)
                {
                        SerialPort_RegWrite (serial, SCL_WH|SDA_R);
                        i2c_delay(DELAYTIME);
                        if((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)
                                break;
                }
                data = (data<<1) + (0x1 & (SerialPort_RegRead(serial)>>SDA_SHIFT));
                SerialPort_RegWrite (serial, SCL_WL|SDA_R); 
                i2c_delay(DELAYTIME/2);
        }
        return data;
}

void i2c_write (unsigned addr)
{
        int i, value;
        int j = 0;
        int maxloop = RETRYTIMES;     

        for (i=7; i>=0; i--)
        {
                value= (0x1 & (addr>>i));

                if (value ==0)
                {
                        SerialPort_RegWrite(serial, SCL_WL|SDA_WL); 
                        i2c_delay(DELAYTIME/2);
                        j = 0;
                        while(j++ < maxloop)
                        {
                                SerialPort_RegWrite(serial, SCL_WH|SDA_WL); 
                                i2c_delay(DELAYTIME);
                                if((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)
                                        break;
                        }      
                        SerialPort_RegWrite(serial, SCL_WL|SDA_WL);
                        i2c_delay(DELAYTIME/2);
                }
                else 
                {
                        SerialPort_RegWrite(serial, SCL_WL|SDA_WH); 
                        i2c_delay(DELAYTIME/2);
                        j = 0;
                        while(j++ < maxloop)
                        {
                                SerialPort_RegWrite(serial, SCL_WH|SDA_WH);
                                i2c_delay(DELAYTIME);
                                if((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)
                                        break;
                        }                  
                        SerialPort_RegWrite(serial, SCL_WL|SDA_WH); 
                        i2c_delay(DELAYTIME/2);
                }
        }
}

int i2c_ack_read()
{
        int ack = 0;
        int j = 0;
        int maxloop = RETRYTIMES;

        SerialPort_RegWrite(serial, SCL_WL|SDA_R); 
        i2c_delay(DELAYTIME);
        while(j++ < maxloop)
        {
                SerialPort_RegWrite(serial, SCL_WH|SDA_R); 
                i2c_delay(DELAYTIME);
                if((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)
                        break;
        }        

        ack = (SDA_BIT & (SerialPort_RegRead(serial))); 
        SerialPort_RegWrite(serial, SCL_WL|SDA_R); 
        i2c_delay(4*DELAYTIME);

        if (ack==0) 
                return TRUE;
        else 
                return FALSE;
}

int i2c_nack_write()
{
        int j = 0;
        int maxloop = RETRYTIMES;    

        SerialPort_RegWrite(serial, SCL_WL|SDA_R); 
        i2c_delay(DELAYTIME);
        while(j++ < maxloop)
        {
                SerialPort_RegWrite(serial, SCL_WH|SDA_R); 
                i2c_delay(DELAYTIME);
                if((SerialPort_RegRead(serial)>>SCL_SHIFT)&0x01)
                        break;
        }           
         
        SerialPort_RegWrite(serial, SCL_WL|SDA_R); 
        i2c_delay(4*DELAYTIME);

        return TRUE;
}

int i2c_ack_write()
{
        int j = 0;
        int maxloop = RETRYTIMES;    

        SerialPort_RegWrite(serial, SCL_WL|SDA_WL); 
        i2c_delay(DELAYTIME);
        while(j ++ < maxloop)
        {
                SerialPort_RegWrite(serial, SCL_WH|SDA_WL); 
                i2c_delay(DELAYTIME);
                if ((SerialPort_RegRead(serial) >> SCL_SHIFT) & 0x01)
                        break;
        }
        SerialPort_RegWrite(serial, SCL_WL|SDA_R); 
        i2c_delay(4*DELAYTIME);

        return TRUE;
}

int i2c_read_data(unsigned char addr, unsigned char *data)
{
        //printf("reading addr: %x\n", addr);
        i2c_start();        // START

        i2c_write(slave_addr);    // slave address & bit 0 = 0 is for write mode
        if(i2c_ack_read()==FALSE) 
                return FALSE;   // wait for ack

        i2c_write(addr&0xff);    // write addr
        if(i2c_ack_read()==FALSE) 
                return FALSE;   // wait for ack

        i2c_start();        // start bit
        i2c_write(slave_addr+1);    // slave address & bit 0 = 1 is for read
        if(i2c_ack_read()==FALSE) 
                return FALSE;   // wait for ack

        *data = i2c_read();

        i2c_nack_write();

        i2c_stop();
        
        return TRUE;
}

int i2c_read_data_page(unsigned char addr, unsigned int len, unsigned char *data)
{
        unsigned int i;
        //printf("reading addr: %x\n", addr);
        i2c_start();        // start bit
        //printf("i2c start is ok");
        i2c_write(slave_addr);    // slave address & bit 0 = 0 is for write mode
        //printf("i2c write slaver_add is finish");
        for(;;){
            if(i2c_ack_read()==FALSE){  //add by cheerychen and to avoid fail;
                i2c_start();
                i2c_write(slave_addr);
             }
             else{
                 break;
             }
        }
        //if(i2c_ack_read()==FALSE) 
        //        return FALSE;   // wait for ack
       //printf("wait for slaver ack\n");
        i2c_write(addr&0xff);    // write addr
        if(i2c_ack_read()==FALSE) 
                return FALSE;   // wait for ack
       // printf("wait for subaddress ack\n");
        i2c_start();        // start bit
        i2c_write(slave_addr+1);    // slave address & bit 0 = 1 is for read
        if(i2c_ack_read()==FALSE) 
                return FALSE;   // wait for ack

                
        //printf("I2C Read start...\n");        
        for (i=0; i<len; i++)
        {
                data[i] = i2c_read();   //add by Janice Yi, Oct. 24, 2007
                if(i==(len-1))
                        i2c_nack_write();
                else
                        i2c_ack_write();
                //printf("I2C Read data index is %d\n",i);
        }

        i2c_stop();
        
        return TRUE;
}

int i2c_write_data(unsigned char addr, unsigned char data)
{
        i2c_start();
        
        i2c_write(slave_addr);
        if(i2c_ack_read()==FALSE) 
        {
                return FALSE;
        }
        
        i2c_write(addr&0xff);
        if(i2c_ack_read()==FALSE) 
                return FALSE;
        
        i2c_write(data);
        if(i2c_ack_read()==FALSE) 
                return FALSE;
        
        i2c_stop();
        
        return TRUE;
}

int i2c_write_data_page(unsigned char addr, unsigned int len, unsigned char *data)
{
        unsigned int i;
		unsigned int flag=0;
        i2c_start();
        //printf("i2c start writing addr: %x\n", addr);
        i2c_write(slave_addr);
       // printf("writing slaver_addr: %x\n", slave_addr);
        for(;;){
            if(i2c_ack_read()==FALSE){
                i2c_start();                      //add cheerychen,2020.11.14
                i2c_write(slave_addr);    //nack and wait time>40ms:continue;one page write cycle is about 10ms.
			    sleep(10);
				flag++;
             }
             else if(i2c_ack_read()==FALSE)&&(flag==4){   //no ack and wait time =40ms->FALSE\ACK FAIL;;
				 printf("iec slaver address ack fail");
                 return FALSE;
             }
			 else if(i2c_ack_read()){     //ack->break loop;
			     break;
			 }
        }
          // return FALSE;
       // printf("slaver_add ack\n");
        i2c_write(addr&0xff);
       // printf("addr ack\n");
        if(i2c_ack_read()==FALSE) 
               return FALSE;
        //printf("I2C write start...\n");
        //printf("len is :%d\n",len);  
        for(i=0;i<len;i++)
        {
                  
               //printf("I2C write data index is %d",i);
               i2c_write(data[i]);
              // printf("I2C write data index is %d",i);
               if(i2c_ack_read()==FALSE){
                        printf("I2C Read data index is %d\n",i);
                        return FALSE;
               }  
                  
               
                        
        }       
        i2c_stop();
        
        return TRUE;
}

//Delays
void i2c_delay(unsigned i)
{

        i = i*4;        //*2  for 32KHz
                        //*4  for 17KHz
                        //*40 for 1.7KHz
        for (i; i>0 ;i--)
        {
                inp(0x100);
                inp(0x100);
                inp(0x100);
                inp(0x100);
                inp(0x100);
        }

}

