#include <Arduino.h>

#include <Wire.h>
#include <math.h>

#define Addr 0x6c //  I2C address

static const float GMP102_CALIB_SCALE_FACTOR[] = {
  1.0E+00,
  1.0E-05,
  1.0E-10,
  1.0E-05,
  1.0E-10,
  1.0E-15,
  1.0E-12,
  1.0E-17,
  1.0E-21 };

long rp;
int cali[18];
double fp[9];
int temp=0;
double pa=0;

void REG_write(int a,int b){
  Wire.beginTransmission(Addr);
  Wire.write(a);  //  選擇暫存器
  Wire.write(b);  //  寫入值
  Wire.endTransmission();  
}

int REG_read1(int a){
  int data1;
  Wire.beginTransmission(Addr);
  Wire.write(a);  //  選擇暫存器
  Wire.endTransmission(0);  
  Wire.requestFrom(Addr, 1); //request 1 byte
  data1 = Wire.read(); //read data
  return data1;
}

void calisave(void){  //Read 18-byte from AAh to BBh and save them
  int i;
  for(i=0;i<18;i++){
    cali[i]=REG_read1(i+0xAA);
    Serial.print(i+0xAA,HEX);Serial.print("h= ");Serial.println(cali[i],HEX);
  }
}

void Pforced(void){
  rp=0L;
  REG_write(0xA5,0x02);
  delay(50);
  REG_write(0x30,0x09);
  do{
    delay(50);
  }while((REG_read1(0x02)&0x01)==0);
  rp=REG_read1(0x06);
  rp=(rp<<8)|REG_read1(0x07);
  rp=(rp<<8)|REG_read1(0x08);
  rp=(rp<<8)>>8;
}

void Tforced(void){
  temp=0;
  REG_write(0xA5,0x00); // set A5h = 0x00
  delay(50);
  REG_write(0x30,0x08); // set 30h = 0x08
  do{
    delay(50);
  }while((REG_read1(0x02)&0x01)==0); //Check 02h[0] (DRDY) bit and wait until its value is set 
  temp=REG_read1(0x09); 
  temp=(temp<<8)|REG_read1(0x0A);  //Read the output from the temperature data registers (09h~0Ah). 
}

void get_fp(void){
  int k;
  int comp;
  int scale_factor; 
  for (k=0;k<9;k++){
    comp=(cali[2*k]<<8)|cali[2*k+1];
    scale_factor=cali[2*k+1]&3;
    comp=comp>>2;
    fp[k]= comp*pow(10,scale_factor)*GMP102_CALIB_SCALE_FACTOR[k];
  } 
}

void rinit(void){
  REG_write(0x00,0x24);  //Set RESET register (00h) to 0x24
  delay(500);
  calisave(); //Read 18-byte from AAh to BBh and save them 
  int b;
  for(b=0;b<4;b++){
    REG_write(b+0xAA,0x00);  //Set four registers AAh ~ ADh to 0x00
  }
  delay(200);
}

void setup()
{
  delay(100);
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Serial_begin");
  rinit();    //  soft_reset
  Serial.println("init_done");
  delay(100);
}

void loop()
{
  Tforced();
  delay(100);
  Pforced();
  delay(100);
  get_fp(); //Get the parameters : fParam[0]~ fParam[8]
  delay(100);
  pa= \
    fp[0] + \
    fp[1]*temp + \
    fp[2]*temp*temp + \
    fp[3]*rp + \
    fp[4]*temp*rp + \
    fp[5]*temp*temp*rp + \
    fp[6]*rp*rp + \
    fp[7]*temp*rp*rp + \
    fp[8]*temp*temp*rp*rp;
  float t =temp;
  Serial.println();Serial.print("temp=");Serial.println(t/256);
  Serial.print("pre=");Serial.println(pa);Serial.println();Serial.println("-----");
}