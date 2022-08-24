#define BIT0    0x01
#define BIT1    0x02
#define BIT2    0x04
#define BIT3    0x08
#define BIT4    0x10
#define BIT5    0x20
#define BIT6    0x40
#define BIT7    0x80

#define S3G_SERIAL_CRT   	0xC5
#define S3G_SERIAL_HDMI0	0xA0
#define S3G_SERIAL_HDMI1	0xAA
#define S3G_SERIAL_HDMI2	0xC6
#define S3G_SERIAL_HDMI3	0xF8

#define DELAYTIME  2
//#define DELAYTIME  100
//#define RETRYTIMES 10000
#define RETRYTIMES 10000
unsigned int g_mmiobase = 0;

void SerialPort_Init(unsigned char serialno, unsigned char slaveaddr);
int i2c_read_data(unsigned char addr, unsigned char *data);
int i2c_read_data_page(unsigned char addr, unsigned int len, unsigned char *data);
int i2c_write_data(unsigned char addr, unsigned char data);
int i2c_write_data_page(unsigned char addr, unsigned int len, unsigned char *data);
void i2c_stop();
unsigned int GetMMIOBase();
