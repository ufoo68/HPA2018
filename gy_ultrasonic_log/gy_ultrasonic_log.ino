#include <Wire.h>
#include "KalmanFilter.h"
#include <SPI.h>
#include <SD.h>
#include <MsTimer2.h>

// 加速度/ジャイロセンサーの制御定数。
#define MPU6050_ADDR         0x68 // MPU-6050 device address
#define MPU6050_SMPLRT_DIV   0x19 // MPU-6050 register address
#define MPU6050_CONFIG       0x1a
#define MPU6050_GYRO_CONFIG  0x1b
#define MPU6050_ACCEL_CONFIG 0x1c
#define MPU6050_ACCEL_XOUT_H 0x3b
#define MPU6050_ACCEL_XOUT_L 0x3c
#define MPU6050_ACCEL_YOUT_H 0x3d
#define MPU6050_ACCEL_YOUT_L 0x3e
#define MPU6050_ACCEL_ZOUT_H 0x3f
#define MPU6050_ACCEL_ZOUT_L 0x40
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_GYRO_XOUT_L  0x44
#define MPU6050_GYRO_YOUT_H  0x45
#define MPU6050_GYRO_YOUT_L  0x46
#define MPU6050_GYRO_ZOUT_H  0x47
#define MPU6050_GYRO_ZOUT_L  0x48
#define MPU6050_PWR_MGMT_1   0x6b
#define MPU6050_WHO_AM_I     0x75
//LPF param
float preY = 0;
float preX = 0;
float a = 0.8;

// sd card
const int chipSelect = 4;
volatile const char *filename = "test.csv";
int wrieInterval = 100;

// ultra sonic
const int Trig = 2;
volatile const int Echo = 3;
volatile float Distance;
volatile unsigned long Duration;
volatile unsigned long bfrTime;
volatile boolean shotFlg = false;

// 加速度/ジャイロセンサーの制御変数。
KalmanFilter gKfx, gKfy; // カルマンフィルタ。
float gCalibrateX, gCalibrateY; // 初期化時の角度。（＝静止角とみなす）
long gPrevMicros; // loop()間隔の計測用。
volatile float degX, degY;

// 加速度/ジャイロセンサーへのコマンド送信。
void writeMPU6050(byte reg, byte data) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

// 加速度/ジャイロセンサーからのデータ読み込み。
byte readMPU6050(byte reg) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 1/*length*/, false);
  byte data =  Wire.read();
  Wire.endTransmission(true);
  return data;
}

void setup() {
  Serial.begin(9600);

  // init for sd card
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    dataFile.println("logging start");
    dataFile.close();
  }
  else {
    Serial.println("error opening");
  }

  // 加速度/ジャイロセンサーの初期化。
  Wire.begin();
  if (readMPU6050(MPU6050_WHO_AM_I) != 0x68) {
    Serial.println("\nWHO_AM_I error.");
    while (true) ;
  }
  writeMPU6050(MPU6050_SMPLRT_DIV, 0x07);   // sample rate: 8kHz/(7+1) = 1kHz
  writeMPU6050(MPU6050_CONFIG, 0x00);       // disable DLPF, gyro output rate = 8kHz
  writeMPU6050(MPU6050_GYRO_CONFIG, 0x00);  // gyro range: ±250dps
  writeMPU6050(MPU6050_ACCEL_CONFIG, 0x00); // accel range: ±2g
  writeMPU6050(MPU6050_PWR_MGMT_1, 0x01);   // disable sleep mode, PLL with X gyro
  delay(2000);

  // 重力加速度から求めた角度をカルマンフィルタの初期値とする。
  float ax = (readMPU6050(MPU6050_ACCEL_XOUT_H) << 8) | readMPU6050(MPU6050_ACCEL_XOUT_L);
  float ay = (readMPU6050(MPU6050_ACCEL_YOUT_H) << 8) | readMPU6050(MPU6050_ACCEL_YOUT_L);
  float az = (readMPU6050(MPU6050_ACCEL_ZOUT_H) << 8) | readMPU6050(MPU6050_ACCEL_ZOUT_L);
  float degRoll  = atan2(ay, az) * RAD_TO_DEG;
  float degPitch = atan(-ax / sqrt(ay * ay + az * az)) * RAD_TO_DEG;
  gKfx.setAngle(degRoll);
  gKfy.setAngle(degPitch);
  gCalibrateY = degPitch;
  gCalibrateX = degRoll;
  gPrevMicros = micros();

  // timer interrupt
  MsTimer2::set(wrieInterval, writeSD);
  MsTimer2::start();

  // setup ultrasonic
  pinMode(Trig,OUTPUT);
  pinMode(Echo,INPUT);
  attachInterrupt(Echo-2, calDuration, CHANGE);
  digitalWrite(Trig,LOW);
  delayMicroseconds(1);
  digitalWrite(Trig,HIGH);
  delayMicroseconds(11);
  digitalWrite(Trig,LOW);
}

void loop() {
  // 重力加速度から角度を求める。
  float ax = (readMPU6050(MPU6050_ACCEL_XOUT_H) << 8) | readMPU6050(MPU6050_ACCEL_XOUT_L);
  float ay = (readMPU6050(MPU6050_ACCEL_YOUT_H) << 8) | readMPU6050(MPU6050_ACCEL_YOUT_L);
  float az = (readMPU6050(MPU6050_ACCEL_ZOUT_H) << 8) | readMPU6050(MPU6050_ACCEL_ZOUT_L);
  float degRoll  = atan2(ay, az) * RAD_TO_DEG;
  float degPitch = atan(-ax / sqrt(ay * ay + az * az)) * RAD_TO_DEG;
  // ジャイロで角速度を求める。
  float gx = (readMPU6050(MPU6050_GYRO_XOUT_H) << 8) | readMPU6050(MPU6050_GYRO_XOUT_L);
  float gy = (readMPU6050(MPU6050_GYRO_YOUT_H) << 8) | readMPU6050(MPU6050_GYRO_YOUT_L);
  float gz = (readMPU6050(MPU6050_GYRO_ZOUT_H) << 8) | readMPU6050(MPU6050_GYRO_ZOUT_L);
  float dpsX = gx / 131.0; // LSB sensitivity: 131 LSB/dps @ ±250dps
  float dpsY = gy / 131.0;
  float dpsZ = gz / 131.0;
  // カルマンフィルタで角度(x,y)を計算する。
  unsigned long curMicros = micros();
  float dt = (float)(curMicros - gPrevMicros) / 1000000; // μsec -> sec
  gPrevMicros = curMicros;
  degX = gKfx.calcAngle(degRoll, dpsX, dt);
  degY = gKfy.calcAngle(degPitch, dpsY, dt);
  degY -= gCalibrateY;
  degX -= gCalibrateX;
  // LPF
  degY = a * preY + (1.0-a) * degY;
  preY = degY;
  degX = a * preX + (1.0-a) * degX;
  preX = degX;

  // distance
  if (shotFlg) {
    shotTrig();
  }
}

void writeSD() {
  String dataString = "";
  dataString += String(degY);
  dataString += ",";
  dataString += String(degX);
  dataString += ",";
  dataString += String(Distance);
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }
  else {
    Serial.println("error opening");
  }
  shotFlg = true;
}

void calDuration() {
  if (digitalRead(Echo)) {
    bfrTime = micros();
  }
  else {
    Duration = micros() - bfrTime;
    Distance = Duration/2;
    Distance = Distance*340*100/1000000; // ultrasonic speed is 340m/s = 34000cm/s = 0.034cm/us
  }
}

void shotTrig() {
  digitalWrite(Trig,LOW);
  delayMicroseconds(1);
  digitalWrite(Trig,HIGH);
  delayMicroseconds(11);
  digitalWrite(Trig,LOW);
  shotFlg = false;
}

