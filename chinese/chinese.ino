/*
  小型太陽能發電系統
*/

#include <Wire.h>  // 使用 Wire 程式庫

#include <LiquidCrystal_I2C.h>  // 使用 LiquidCrystal_I2C 程式庫
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include "DHT.h"  // 使用 DHT 程式庫
// DHT11 輸出腳
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include <BH1750.h>  // 使用 BH1750 程式庫
BH1750 lightMeter;

#include <Servo.h>  // 使用 Servo 程式庫
Servo lr_servo;  // 定義控制左右轉動伺服馬達的名稱
Servo ud_servo;  // 定義控制上下轉動伺服馬達的名稱

const byte interruptPin = 2;  // 按鈕的引腳，用於干擾中斷
int lr_angle = 90;  // 將初始角度設置為 90 度
int ud_angle = 10;  // 將初始角度設置為 10 度，保持太陽能板垂直以檢測最強光線
int l_state = A0;  // 定義光敏電阻的類比電壓輸入引腳
int r_state = A1;
int u_state = A2;
int d_state = A3;
const byte buzzer = 6;  // 將蜂鳴器的引腳設置為數位引腳 6
const byte lr_servopin = 9;  // 定義控制左右轉動伺服馬達的引腳
const byte ud_servopin = 10;  // 定義控制上下轉動伺服馬達的引腳

unsigned int light;  // 保存光線強度的變數
byte error = 15;  // 定義誤差範圍以防止振動
byte m_speed = 10;  // 設置延遲時間以調整伺服馬達的速度，時間越長，速度越慢
byte resolution = 1;  // 設置伺服馬達的旋轉精度，最小旋轉角度
int h;  // 濕度
int t;  // 攝氏溫度
int f;  // 華氏溫度

void setup() {
  Serial.begin(9600);  // 定義串行通信的波特率
  // 初始化 I2C 总线（BH1750 程式庫不會自動執行此操作）
  Wire.begin();
  lightMeter.begin();
  Serial.println("DHT11 test!");
  dht.begin();  // 啟動DHT

  lr_servo.attach(lr_servopin);  // 設置伺服馬達的控制引腳
  ud_servo.attach(ud_servopin);  // 設置伺服馬達的控制引腳
  pinMode(l_state, INPUT);  // 設置引腳模式
  pinMode(r_state, INPUT);
  pinMode(u_state, INPUT);
  pinMode(d_state, INPUT);

  pinMode(interruptPin, INPUT_PULLUP);  // 設置按鈕引腳為輸入上拉模式
  attachInterrupt(digitalPinToInterrupt(interruptPin), adjust_resolution, FALLING);  // 外部中斷觸摸類型為下降沿；adjust_resolution 是中斷服務函數 ISR

  lcd.init();  // 初始化 LCD
  lcd.backlight();  // 設置 LCD 背光

  lr_servo.write(lr_angle);  // 返回初始角度
  delay(2000);
  ud_servo.write(ud_angle);
  delay(2000);
}

void loop() {
  ServoAction();  // 伺服馬達執行動作
  read_light();  // 讀取 BH1750 的光強度
  read_dht11();  // 讀取溫度和濕度的值
  LcdShowValue();  // LCD 顯示光強度、溫度和濕度的值
}

/********** 伺服馬達的功能 ************/
void ServoAction() {
  int L = analogRead(l_state);  // 讀取傳感器的類比電壓值，範圍 0-1023
  int R = analogRead(r_state);
  int U = analogRead(u_state);
  int D = analogRead(d_state);
  
  /********************** 調整左右 **********************/
  if (abs(L - R) > error && L > R) {  // 判斷誤差是否在可接受範圍內，否則調整伺服馬達
    lr_angle -= resolution;  // 減少角度
    if (lr_angle < 0) {  // 限制伺服馬達的旋轉角度
      lr_angle = 0;
    }
    lr_servo.write(lr_angle);  // 輸出伺服馬達的角度
    delay(m_speed);
  }
  else if (abs(L - R) > error && L < R) {  // 判斷誤差是否在可接受範圍內，否則調整伺服馬達
    lr_angle += resolution;  // 增加角度
    if (lr_angle > 180) {  // 限制伺服馬達的旋轉角度
      lr_angle = 180;
    }
    lr_servo.write(lr_angle);  // 輸出伺服馬達的角度
    delay(m_speed);
  }
  else if (abs(L - R) <= error) {  // 判斷誤差是否在可接受範圍內，否則調整伺服馬達
    lr_servo.write(lr_angle);  // 輸出伺服馬達的角度
  }
  
  /********************** 調整上下 **********************/
  if (abs(U - D) > error && U >= D) {  // 判斷誤差是否在可接受範圍內，否則調整伺服馬達
    ud_angle -= resolution;  // 減少角度
    if (ud_angle < 10) {  // 限制伺服馬達的旋轉角度
      ud_angle = 10;
    }
    ud_servo.write(ud_angle);  // 輸出伺服馬達的角度
    delay(m_speed);
  }
  else if (abs(U - D) > error && U < D) {  // 判斷誤差是否在可接受範圍內，否則調整伺服馬達
    ud_angle += resolution;  // 增加角度
    if (ud_angle > 90) {  // 限制伺服馬達的旋轉角度
      ud_angle = 90;
    }
    ud_servo.write(ud_angle);  // 輸出伺服馬達的角度
    delay(m_speed);
  }
  else if (abs(U - D) <= error) {  // 判斷誤差是否在可接受範圍內，如果是，保持穩定，角度不變
    ud_servo.write(ud_angle);  // 輸出伺服馬達的角度
  }
}

void read_dht11() {
    h = dht.readHumidity();  // 讀取濕度
    t = dht.readTemperature();  // 讀取攝氏溫度
    f = dht.readTemperature(true);  // 讀取華氏溫度
 
    if (isnan(h) || isnan(t) || isnan(f)) {
        Serial.println("無法從 DHT 模組讀取資料!");
        return;
    }
    Serial.print("濕度:");  // 在序列監控視窗顯示溫度與濕度
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
  dtostrf(light, -5, 0, str1);  // 將光強度值格式化為字符串，左對齊
  dtostrf(t, -2, 0, str2);
  dtostrf(h, -2, 0, str3);
  
  // 顯示光強度的值
  lcd.setCursor(0, 0);
  lcd.print("Light:");
  lcd.setCursor(6, 0);
  lcd.print(str1);
  lcd.setCursor(11, 0);
  lcd.print("lux");
  
  // 顯示溫度和濕度的值
  lcd.setCursor(0, 1);
  lcd.print(t);
  lcd.setCursor(2, 1);
  lcd.print("C");
  lcd.setCursor(5, 1);
  lcd.print(h);
  lcd.setCursor(7, 1);
  lcd.print("%");

  // 顯示旋轉的精度
  lcd.setCursor(11, 1);
  lcd.print("res:");
  lcd.setCursor(15, 1);
  lcd.print(resolution);
}

void read_light() {
  light = lightMeter.readLightLevel();  // 讀取 BH1750 感測器偵測到的光強度
}

/********* 中斷服務函數 **************/
void adjust_resolution() {
  tone(buzzer, 800, 100);
  delay(10);  // 延遲以消除振動
  if (!digitalRead(interruptPin)){
    if(resolution < 5){
      resolution++;
    } else {
      resolution = 1;
    }
  }
}