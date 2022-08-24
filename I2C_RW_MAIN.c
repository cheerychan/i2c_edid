/****************************************************/
/*      Function        :       S3G EDID Analysis                           */
/*      Author          :       cheerychen                                 */
/*      Date            :       nov 11th, 2020                              */
/*      Rev.            :       1.0.0.1                                      */
/*      Compiler        :       Watcom(DOS/4GW)                               */
/****************************************************/
#include<stdio.h>
#include<limits.h>
#include<string.h>
#include<stdlib.h>
#include"i2c.h"
//#include "def.h"

#define EDID_DEVADDR               0xa0
#define EDID_DP_DEVADDR            0x50
#define EDID_DP_SEGMENT_ADDR       0x30

#define EDID_ONE_BLOCK_SIZE                 128
#define EDID_BUFFER_SIZE                    2 * EDID_ONE_BLOCK_SIZE
#define TRUE 1
#define FALSE 0

unsigned int I2cReadData(unsigned char serialNo, unsigned char deviceAddr, unsigned int length, unsigned char * buffer)
{
        
        unsigned int i;
        memset(buffer, 0, length);
        
        SerialPort_Init(serialNo, deviceAddr);

        if (i2c_read_data_page(0x00, length, buffer) == FALSE)
        {
                i2c_stop();
                printf("\nERROR: I2C Read error! Can not find the Device!\n");
                
                return FALSE;
        }
     
                
                        printf("\n");
                        printf("    ");
                        for (i = 0; i < 16; i ++)
                        {
                                printf ("%02x  ", i);
                        }
                        printf("\n");

                        for (i = 0x00; i <= 0xff; i++)  //read back
                        {
                                if ((i % 16) == 0)
                                {
                                        printf("\n");
                                        printf ("%02x  ", i);
                                }

                                printf("%02x  ", buffer[i]);                           
                        }
                        printf("\n");
      
    

        return TRUE;
}


unsigned int  I2cWriteData(unsigned int length, unsigned char * data,unsigned int DeviceType ){
        unsigned int  i2cAddr;
        unsigned int x;
        unsigned char data_temp[3]="";
        unsigned int compare[256]={0};
        unsigned int ndata=0x00;
        unsigned char subAddr=0;
        unsigned int n=0;unsigned int h=0;
        unsigned int m=0;unsigned int k=0;
        unsigned int p=0;
       unsigned int flag=0;unsigned int mismatch[256]={0};
        unsigned char data_page[8]={0};
        unsigned int len=0;
        unsigned char *buffer = (unsigned char*)malloc(sizeof(unsigned char) * EDID_BUFFER_SIZE);
        switch(DeviceType)
        {
               case 0: i2cAddr = S3G_SERIAL_CRT;break;
               case 1: i2cAddr = S3G_SERIAL_HDMI0;break;
               case 2: i2cAddr = S3G_SERIAL_HDMI1;break;
               default: CmdUsage();break;
        }
//send_data
       printf("start i2c send\n");
       /*
       for(x=0;x<length;x+=2){
            //SerialPort_Init(i2cAddr, EDID_DEVADDR);
            data_temp[0]=data[x];
            data_temp[1]=data[x+1];
            data_temp[2]='\0';
            printf("data_temp:%s",data_temp);
            ndata=StoHn(data_temp);
            printf("\nI2C write value = 0x%2.2x\n", ndata);
            SerialPort_Init(i2cAddr, EDID_DEVADDR);
            if(i2c_write_data(subAddr,ndata)){
                printf("I2C write ADD = 0x%2.2x\n", subAddr);
                printf("\nI2C write value = 0x%2.2x\n", ndata);
                //i2c_stop();
            }
            else{
                i2c_stop();
                printf("I2C write wrong\n");
            }
            subAddr=subAddr+1;
            printf("I2C will write ADD = 0x%2.2x\n", subAddr);
            compare[n]=ndata;
            n++;
         
       }
       */
       
       for(x=0;x<length;x+=2){
            //SerialPort_Init(i2cAddr, EDID_DEVADDR);
            data_temp[0]=data[x];
            data_temp[1]=data[x+1];
            data_temp[2]='\0';
            //printf("data_temp:%s",data_temp);
            ndata=StoHn(data_temp);
            data_page[p]=ndata;
            compare[n]=ndata;
            n++;
            if((p+1)%8==0){
                SerialPort_Init(i2cAddr, EDID_DEVADDR);
                if (i2c_write_data_page(subAddr,8,data_page) == FALSE)     //i2c_write_data_page(unsigned char addr, unsigned int len, unsigned char *data)
              {
                i2c_stop();
                //printf("subaddress is: 0x%x",subAddr);
                //printf("data_page[0] is:0x%x",data_page[0]);
                  printf("\nERROR: I2C write error! Can not find the Device!\n");
                
                //return -1;  // error exit;
              }
                p=0;
                subAddr=subAddr+8;
            }
            else{
                p++;
            }
            
            
       }
       
      
       
        
//read data_edid;
        memset(buffer, 0, EDID_BUFFER_SIZE);         
        //SerialPort_Init(i2cAddr, EDID_DEVADDR);       
        printf("\nserialPortNo is 0x%2.2x, device address is 0x%2.2x.\n", i2cAddr, EDID_DEVADDR);         
        if (I2cReadData(i2cAddr,EDID_DEVADDR, EDID_BUFFER_SIZE, buffer) == FALSE)
        {
                   i2c_stop();
                   printf("\nERROR: I2C Read error! Can not find the Device!\n");
                                
                   //return FALSE;
        }
        
//add check is xml and edid is right;
        for(m=0x00;m<=0xff;m++){
           if(compare[m]==buffer[m]){
                   continue;
               }
            else{
               flag=1;
               mismatch[k]=m;
               k++;
            }
        }
        if(flag==1){
            printf("xml file and edid file back don't match,please check\n");
            printf("wrong index is following:");
            for(h;h<k;h++){
                printf(" 0x%x ",mismatch[h]);
             }
        }
        else{
            printf("the data of xml file and edid file read back match.i2c write sucessfully\n");
        }
        return 0;
        
         //return 0;
}
 int StoHn(unsigned char s[])
{
        unsigned int hdata=0;
        unsigned int i = 0;
       // hdata = 0;
        while (s[i] != '\0')
        {
                if (s[i] >= '0' && s[i] <= '9')
                        hdata = hdata * 16 + (s[i] - 0x30);
                else if (s[i] >= 'a' && s[i] <= 'f')
                        hdata = hdata * 16 + (s[i] - 0x61 + 0x0a);
                else if (s[i] >= 'A' && s[i] <= 'F')
                        hdata = hdata * 16 + (s[i] - 0x41 + 0x0a);

                i++;
        }
        return hdata;
}
unsigned int StoH(unsigned char * s)
{
        unsigned int hdata;
        unsigned int i = 0;
        hdata = 0;
        while (s[i] != '\0')
        {
                if (s[i] >= '0' && s[i] <= '9')
                        hdata = hdata * 16 + (s[i] - 0x30);
                else if (s[i] >= 'a' && s[i] <= 'f')
                        hdata = hdata * 16 + (s[i] - 0x61 + 0x0a);
                else if (s[i] >= 'A' && s[i] <= 'F')
                        hdata = hdata * 16 + (s[i] - 0x41 + 0x0a);

                i++;
        }
        return hdata;
}
void CmdUsage(void)
{
        printf("Legal Command:\n");
        printf(" -f <xml file> <i2c_address>:0->CRT 1->hDMI/DP0 2->HDMI/DP1.\n");     
        fflush (stdout);
}



int main(int argc, char *argv[]){
        FILE *fp=NULL;
       unsigned int DeType=1;
       unsigned char buff[1000];
       unsigned char data[1000];
      unsigned char temp[1000];
      unsigned char xml_block[7]="BLOCK";
      unsigned int len_buff=0;
      unsigned int len_data=0;
      unsigned int k=0;
      unsigned int i=0;
      unsigned int j=0;
      g_mmiobase = GetMMIOBase();
      if(g_mmiobase == -1)
      {
            printf("Get mmiobase address error!ChipID mismatch!\n ");
            return FALSE;
      }
      printf("MMIO Base Addr = 0x%8.8x\n", g_mmiobase);

   //operate xml file;
   if (argc >3 ){
        if (strcmp(argv[1], "-F") == 0 || strcmp(argv[1], "-f") == 0)
        {
              if ((fp= fopen(argv[2], "rb")) == NULL)     //argv[2]:xml path:"f:/c_test/980_225MHZ.xml"
                        {
                                printf("Cannot open xml file!\n");
                                printf("Please check the filename and path!\n");
                                printf("argv[2] is:%s;\n",argv[2]);
                                return -1;
                        }
  //analyse xml
            for(;;){
                    fgets(buff,1000,(FILE *)fp);
                     if (strcmp(buff,temp)==0){
                            break;    //stop read line
                     }
                     for(i=0;i<1000;i++){
                        if (buff[i]<0){
                               break;    //judge char
                          }
                         else{
                             temp[i]=buff[i];
                        }
                     }
 //judge block
                     if(strstr(buff,xml_block)==NULL){
                            continue;
                     }
                    else{
                          len_buff = strlen(buff);
                          //printf("buff lenth is:%d\n",len_buff);
 //date assign;
                          for (j=0;j<len_buff-19;j++){
                               if(buff[j]==32){  //except ""
                                   continue;
                                  }
                                else{
                                       data[k]=buff[j+8]; //except block0
                                        k++;
                                 }
                           }
                    }
          
          }
          printf("xml date :%s\n",data);
          len_data= strlen(data);
          printf("xml date length is:%d\n",len_data);
//check xml data is right;
          if(len_data!=512){
              printf("xml data length is not 512,please check it.\n ");
              return 0;
          }
          fclose(fp);
//i2c write;
           DeType = StoH(argv[3]); //choose seial hdmi/vga/dp;  eg:vga:0/1/2
          // printf("dwtype is:%d\n",DeType);
           I2cWriteData(len_data, data, DeType);
           //printf("i2c write is finished\n");
           
       }        

        
   }
  
   else{
       CmdUsage();
   }    
  
      return 0;
}









































