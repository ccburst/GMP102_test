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

// write value b to register a
void REG_write(int a,int b){ 
  Wire.beginTransmission(Addr);
  Wire.write(a);  
  Wire.write(b); 
  Wire.endTransmission();  
}

//return value from register a
int REG_read1(int a){ 
  int data1;
  Wire.beginTransmission(Addr);
  Wire.write(a);  
  Wire.endTransmission(0);  
  Wire.requestFrom(Addr, 1); 
  data1 = Wire.read(); 
  return data1;
}

// Read 18-byte from AAh to BBh and save to cali[]
void calisave(void){  
  int i;
  for(i=0;i<18;i++){
    cali[i]=REG_read1(i+0xAA);
  }
}


// Get the raw pressure and save as rp
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

// Get the calibrated temperature and save as temp
void Tforced(void){
  temp=0;
  REG_write(0xA5,0x00); 
  delay(50);
  REG_write(0x30,0x08); 
  do{
    delay(50);
  }while((REG_read1(0x02)&0x01)==0); 
  temp=REG_read1(0x09); 
  temp=(temp<<8)|REG_read1(0x0A);  
}

// Get the parameters : fp[0]~ fp[8]
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

// soft_reset
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
  rinit();   
  Serial.println("init_done");
  delay(100);
}
void loop()
{
  Tforced();
  delay(100);
  Pforced();
  delay(100);
  get_fp(); 
  delay(100);
  
  // Calculate temperature compensated pressure
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
  float a1=44307.694;
  float a2=0.190284;
  float a3=8.33;
  float high=a1*(1-pow((pa/101325),a2));
  Serial.println();Serial.print("temperature=");Serial.println(t/256);
  Serial.print("pressure=");Serial.println(pa);
  Serial.print("heigt(Method 1)=");Serial.println(high);
  Serial.print("heigt(Method 2)=");Serial.println(a3*(pa-101325));
  Serial.println();Serial.println("-----");
}