#include <Wire.h>  
#include <LiquidCrystal_I2C.h>  
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include "DHT.h"  
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#include <BH1750.h> 
BH1750 lightMeter;
#include <Servo.h>  
Servo lr_servo;  
Servo ud_servo;  
const byte interruptPin = 2;  
int lr_angle = 90;
int ud_angle = 10;
int l_state = A0;
int r_state = A1;
int u_state = A2;
int d_state = A3;
const byte buzzer = 6;
const byte lr_servopin = 9;
const byte ud_servopin = 10;
unsigned int light;
byte error = 15;
byte m_speed = 10;
byte resolution = 1;
int h;
int t;
int f;

void setup() {
  Serial.begin(9600); 
  Wire.begin();
  lightMeter.begin();
  Serial.println("DHT11 test!");
  dht.begin(); 
  lr_servo.attach(lr_servopin);
  ud_servo.attach(ud_servopin);
  pinMode(l_state, INPUT);
  pinMode(r_state, INPUT);
  pinMode(u_state, INPUT);
  pinMode(d_state, INPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), adjust_resolution, FALLING);
  lcd.init();
  lcd.backlight();
  lr_servo.write(lr_angle);
  delay(2000);
  ud_servo.write(ud_angle);
  delay(2000);
}
void loop() {
  ServoAction();
  read_light();
  read_dht11();
  LcdShowValue();
}
void ServoAction() {
  int L = analogRead(l_state);
  int R = analogRead(r_state);
  int U = analogRead(u_state);
  int D = analogRead(d_state);
  if (abs(L - R) > error && L > R) {  
    lr_angle -= resolution;
    if (lr_angle < 0) { 
      lr_angle = 0;
    }
    lr_servo.write(lr_angle);
    delay(m_speed);
  }
  else if (abs(L - R) > error && L < R) { 
    lr_angle += resolution;
    if (lr_angle > 180) {
      lr_angle = 180;
    }
    lr_servo.write(lr_angle);
    delay(m_speed);
  }
  else if (abs(L - R) <= error) {
    lr_servo.write(lr_angle);
  }
  if (abs(U - D) > error && U >= D) {
    ud_angle -= resolution;
    if (ud_angle < 10) {
      ud_angle = 10;
    }
    ud_servo.write(ud_angle);
    delay(m_speed);
  }
  else if (abs(U - D) > error && U < D) {
    ud_angle += resolution;
    if (ud_angle > 90) {
      ud_angle = 90;
    }
    ud_servo.write(ud_angle);
    delay(m_speed);
  }
  else if (abs(U - D) <= error) {
    ud_servo.write(ud_angle);
  }
}

void read_dht11() {
    h = dht.readHumidity();
    t = dht.readTemperature();
    f = dht.readTemperature(true);
 
    if (isnan(h) || isnan(t) || isnan(f)) {
        Serial.println("無法從 DHT 模組讀取資料!");
        return;
    }
    Serial.print("濕度:");  
    Serial.print(h);
    Serial.print("%\t");
    Serial.print("溫度:");
    Serial.print(t);
    Serial.print("℃ ");
    Serial.print(f);
    Serial.println("℉");
}

void LcdShowValue() {
  char str1[5];
  char str2[2];
  char str3[2];
  dtostrf(light, -5, 0, str1);
  dtostrf(t, -2, 0, str2);
  dtostrf(h, -2, 0, str3);
  lcd.setCursor(0, 0);
  lcd.print("Light:");
  lcd.setCursor(6, 0);
  lcd.print(str1);
  lcd.setCursor(11, 0);
  lcd.print("lux");
  lcd.setCursor(0, 1);
  lcd.print(t);
  lcd.setCursor(2, 1);
  lcd.print("C");
  lcd.setCursor(5, 1);
  lcd.print(h);
  lcd.setCursor(7, 1);
  lcd.print("%");
  lcd.setCursor(11, 1);
  lcd.print("res:");
  lcd.setCursor(15, 1);
  lcd.print(resolution);
}
void read_light() {
  light = lightMeter.readLightLevel();
}
void adjust_resolution() {
  tone(buzzer, 800, 100);
  delay(10);
  if (!digitalRead(interruptPin)){
    if(resolution < 5){
      resolution++;
    } else {
      resolution = 1;
    }
  }
}
