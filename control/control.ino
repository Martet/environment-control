
#include <SPI.h>
#include <ILI9341_due_config.h>
#include <ILI9341_due.h>
#include "fonts/SystemFont5x7.h"
#include "fonts/Arial_bold_14.h"
#include "URTouch.h"      
#include <DHT.h>    
#include <ILI9341_due_Buttons.h>
#include <DS3231.h>
#include <Wire.h>
#include "extEEPROM.h"
#include <RTCDue.h>
RTCDue rtc(RC);



#define TFT_DC 46
#define TFT_CS 44          
#define t_SCK 50             
#define t_CS 51               
#define t_MOSI 52              
#define t_MISO 53           
#define t_IRQ 49   
#define DHT_pin 36
#define relay1 11
#define relay2 10
#define relay3 9
#define relay4 8

ILI9341_due tft = ILI9341_due(TFT_CS, TFT_DC);

DHT dht(DHT_pin, DHT22);

URTouch myTouch(t_SCK, t_CS, t_MOSI, t_MISO, t_IRQ);
ILI9341_due_Buttons  myButtons(&tft, &myTouch);

DS3231 Clock;
extEEPROM eeprom(kbits_32, 1, 32, 0x57);

unsigned long lastmillisTemp, lastmillisTime;
byte onTime, offTime;
float targetTemp, targetHum;
byte forceTemp, forceHum, forceTime;

gTextArea tempArea{ 10, 10, 295, 20 };
gTextArea humArea{ 10, 35, 295, 20 };
gTextArea targetTempArea{ 10, 60, 70, 20 };
gTextArea targetHumArea{ 85, 60, 70, 20 };
gTextArea timeOnArea{ 160, 60, 70, 20 };
gTextArea timeOffArea{ 235, 60, 70, 20 };
gTextArea tempStateArea{ 10, 125, 70, 40 };
gTextArea humStateArea{ 85, 125, 70, 40 };
gTextArea timeOnLabelArea{ 160, 125, 70, 20 };
gTextArea timeOffLabelArea{ 235, 125, 70, 20 };
gTextArea timeStateArea{ 160, 142, 145, 23 };
gTextArea timeArea{ 240, 210, 80, 15 };
gTextArea dateArea{ 240, 225, 80, 15 };

union {
        float fval;
        byte bval[4];
      } floatAsBytes;

void setup() {
  Serial.begin(9600);
  dht.begin();
  tft.begin();  
  tft.setRotation(iliRotation270);
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  tft.setFont(Arial_bold_14);
  myButtons.setTextFont(Arial_bold_14);
  Wire.begin();
  eeprom.begin(eeprom.twiClock100kHz);
  bool h12, PM, C;
  //setTime(Clock.getHour(h12, PM),Clock.getMinute(),Clock.getSecond(),Clock.getDate(),Clock.getMonth(C),Clock.getYear());
  rtc.begin();
  rtc.setTime(Clock.getHour(h12, PM),Clock.getMinute(),Clock.getSecond());
  rtc.setDate(Clock.getDate(),Clock.getMonth(C),Clock.getYear());
  byte in[4];
  eeprom.read(0, in, 4);
  memcpy(floatAsBytes.bval, in, sizeof(in));
  targetTemp = floatAsBytes.fval;
  eeprom.read(4, in, 4);
  memcpy(floatAsBytes.bval, in, sizeof(in));
  targetHum = floatAsBytes.fval;
  onTime = eeprom.read(10);
  offTime = eeprom.read(11);
  forceTemp = eeprom.read(12);
  forceHum = eeprom.read(13);
  forceTime = eeprom.read(14);
  refreshTargetTemp();
  refreshTargetHum();
  refreshTargetTime();
  refreshTemps();
  refreshTime();
  
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);
  
}



void loop() {
  int plusTempBut = myButtons.addButton( 10,  85, 35,  35, "+");
  int minusTempBut = myButtons.addButton( 45,  85, 35,  35, "-");
  int forceOnTempBut = myButtons.addButton( 10,  170, 35,  35, "ON");
  int forceOffTempBut = myButtons.addButton( 45,  170, 35,  35, "OFF");
  int forceResetTempBut = myButtons.addButton( 10,  205, 35,  35, "RST");
  int plusHumBut = myButtons.addButton( 85,  85, 35,  35, "+");
  int minusHumBut = myButtons.addButton( 120,  85, 35,  35, "-");
  int forceOnHumBut = myButtons.addButton( 85,  170, 35,  35, "ON");
  int forceOffHumBut = myButtons.addButton( 120,  170, 35,  35, "OFF");
  int forceResetHumBut = myButtons.addButton( 85,  205, 35,  35, "RST");
  int plusOnBut = myButtons.addButton( 160,  85, 35,  35, "+");
  int minusOnBut = myButtons.addButton( 195,  85, 35,  35, "-");
  int plusOffBut = myButtons.addButton( 235,  85, 35,  35, "+");
  int minusOffBut = myButtons.addButton( 270,  85, 35,  35, "-");
  int forceOnTimeBut = myButtons.addButton( 160,  170, 35,  35, "ON");
  int forceOffTimeBut = myButtons.addButton( 195,  170, 35,  35, "OFF");
  int forceResetTimeBut = myButtons.addButton( 160,  205, 35,  35, "RST");
  myButtons.drawButtons();

  tft.setTextArea(timeOnLabelArea);
  tft.clearTextArea(ILI9341_BLACK);
  tft.print("ON TIME");
  tft.setTextArea(timeOffLabelArea);
  tft.clearTextArea(ILI9341_BLACK);
  tft.print("OFF TIME");
  tft.setTextArea(0, 0, 320, 240);

  while(1){
  if (myTouch.dataAvailable() == true){
    int pressed_button = myButtons.checkButtons();

    if(pressed_button == plusTempBut){
      targetTemp += 0.1;
      floatAsBytes.fval = targetTemp;
      eeprom.write(0, floatAsBytes.bval, 4);
      refreshTargetTemp();
      refreshTemps();
    }
    else if(pressed_button == minusTempBut){
      targetTemp -= 0.1;
      floatAsBytes.fval = targetTemp;
      eeprom.write(0, floatAsBytes.bval, 4);
      refreshTargetTemp();
      refreshTemps();
    }
    else if(pressed_button == plusHumBut){
      targetHum += 0.1;
      floatAsBytes.fval = targetHum;
      eeprom.write(4, floatAsBytes.bval, 4);
      refreshTargetHum();
      refreshTemps();
    }
    else if(pressed_button == minusHumBut){
      targetHum -= 0.1;
      floatAsBytes.fval = targetHum;
      eeprom.write(4, floatAsBytes.bval, 4);
      refreshTargetHum();
      refreshTemps();
    }
    else if(pressed_button == plusOnBut){
      onTime += 1;
      if(onTime > 23) onTime = 0;
      eeprom.write(10, onTime);
      refreshTargetTime();
      refreshTime();
    }
    else if(pressed_button == minusOnBut){
      onTime -= 1;
      if(onTime > 23) onTime = 23;
      eeprom.write(10, onTime);
      refreshTargetTime();
      refreshTime();
    }
    else if(pressed_button == plusOffBut){
      offTime += 1;
      if(offTime > 23) offTime = 0;
      eeprom.write(11, offTime);
      refreshTargetTime();
      refreshTime();
    }
    else if(pressed_button == minusOffBut){
      offTime -= 1;
      if(offTime > 23) offTime = 23;
      eeprom.write(11, offTime);
      refreshTargetTime();
      refreshTime();
    }
    else if(pressed_button == forceOnTempBut){
      forceTemp = 1;
      eeprom.write(12, forceTemp);
      refreshTemps();
    }
    else if(pressed_button == forceOffTempBut){
      forceTemp = 2;
      eeprom.write(12, forceTemp);
      refreshTemps();
    }
    else if(pressed_button == forceResetTempBut){
      forceTemp = 0;
      eeprom.write(12, forceTemp);
      refreshTemps();
    }
    else if(pressed_button == forceOnHumBut){
      forceHum = 1;
      eeprom.write(13, forceHum);
      refreshTemps();
    }
    else if(pressed_button == forceOffHumBut){
      forceHum = 2;
      eeprom.write(13, forceHum);
      refreshTemps();
    }
    else if(pressed_button == forceResetHumBut){
      forceHum = 0;
      eeprom.write(13, forceHum);
      refreshTemps();
    }
    else if(pressed_button == forceOnTimeBut){
      forceTime = 1;
      eeprom.write(14, forceTime);
      refreshTime();
    }
    else if(pressed_button == forceOffTimeBut){
      forceTime = 2;
      eeprom.write(14, forceTime);
      refreshTime();
    }
    else if(pressed_button == forceResetTimeBut){
      forceTime = 0;
      eeprom.write(14, forceTime);
      refreshTime();
    }
  }
  
  if(millis() - lastmillisTemp >= 2000){
    refreshTemps();

    lastmillisTemp = millis();
  }

  if(millis() - lastmillisTime >= 1000){
    refreshTime();

    lastmillisTime = millis();
  }

}}

void refreshTargetTemp(){
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextArea(targetTempArea);
  tft.clearTextArea(ILI9341_BLACK);
  tft.print(String(targetTemp) + "C");
  tft.setTextArea(0, 0, 320, 240);
}

void refreshTargetHum(){
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextArea(targetHumArea);
  tft.clearTextArea(ILI9341_BLACK);
  tft.print(String(targetHum) + "%");
  tft.setTextArea(0, 0, 320, 240);
}

void refreshTargetTime(){
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextArea(timeOnArea);
  tft.clearTextArea(ILI9341_BLACK);
  tft.print(String(onTime) + " : 00");
  tft.setTextArea(timeOffArea);
  tft.clearTextArea(ILI9341_BLACK);
  tft.print(String(offTime) + " : 00");
  tft.setTextArea(0, 0, 320, 240);
}

void refreshTemps(){
  float tep = dht.readTemperature();
    float vlh = dht.readHumidity();
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextArea(tempArea);
    tft.clearTextArea(ILI9341_BLACK);
    tft.print("TEMPERATURE: ");
    tft.print(String(tep) + "C");
    tft.setTextArea(humArea);
    tft.clearTextArea(ILI9341_BLACK);
    tft.print("HUMIDITY: ");
    tft.print(String(vlh) + "%");
    if(forceTemp == 1){
      tft.setTextArea(tempStateArea);
      tft.clearTextArea(ILI9341_GREEN);
      tft.print("HEATING:\n");
      tft.print("ON!!!");
      digitalWrite(relay1, LOW);
    }
    else if(forceTemp == 2){
      tft.setTextArea(tempStateArea);
      tft.clearTextArea(ILI9341_RED);
      tft.print("HEATING:\n");
      tft.print("OFF!!!");
      digitalWrite(relay1, HIGH);
    }
    else{
      if(tep < targetTemp){
        tft.setTextArea(tempStateArea);
        tft.clearTextArea(ILI9341_GREEN);
        tft.print("HEATING:\n");
        tft.print("ON");
        digitalWrite(relay1, LOW);
      }
      else{
        tft.setTextArea(tempStateArea);
        tft.clearTextArea(ILI9341_RED);
        tft.print("HEATING:\n");
        tft.print("OFF");
        digitalWrite(relay1, HIGH);
      }
    }
    if(forceHum == 1){
      tft.setTextArea(humStateArea);
      tft.clearTextArea(ILI9341_GREEN);
      tft.print("MISTER:\n");
      tft.print("ON!!!");
      digitalWrite(relay2, LOW);
    }
    else if(forceHum == 2){
      tft.setTextArea(humStateArea);
      tft.clearTextArea(ILI9341_RED);
      tft.print("MISTER:\n");
      tft.print("OFF!!!");
      digitalWrite(relay2, HIGH);
    }
    else{
      if(vlh < targetHum){
        tft.setTextArea(humStateArea);
        tft.clearTextArea(ILI9341_GREEN);
        tft.print("MISTER:\n");
        tft.print("ON");
        digitalWrite(relay2, LOW);
      }
      else{
        tft.setTextArea(humStateArea);
        tft.clearTextArea(ILI9341_RED);
        tft.print("MISTER:\n");
        tft.print("OFF");
        digitalWrite(relay2, HIGH);
      }
    }
    tft.setTextArea(0, 0, 320, 240);
}

void refreshTime(){
  tft.setTextArea(timeArea);
    tft.clearTextArea(ILI9341_BLACK);
    tft.print(rtc.getHours(), DEC);
    tft.print(" : ");
    tft.print(rtc.getMinutes(), DEC);
    tft.print(" : ");
    tft.print(rtc.getSeconds(), DEC);
    tft.setTextArea(dateArea);
    tft.clearTextArea(ILI9341_BLACK);
    tft.print(rtc.getDay(), DEC);
    tft.print(". ");
    tft.print(rtc.getMonth(), DEC);
    tft.print(". ");
    tft.print(rtc.getYear(), DEC);
    tft.setTextArea(timeStateArea);
    if(forceTime == 1){
      tft.clearTextArea(ILI9341_GREEN);
      tft.setFontMode(gTextFontModeTransparent);
      tft.print("    ");
      tft.setFontMode(gTextFontModeSolid);
      tft.print("LIGHTS: ON!!!");
      digitalWrite(relay3, LOW);
    }
    else if(forceTime == 2){
      tft.clearTextArea(ILI9341_RED);
      tft.setFontMode(gTextFontModeTransparent);
      tft.print("    ");
      tft.setFontMode(gTextFontModeSolid);
      tft.print("LIGHTS: OFF!!!");
      digitalWrite(relay3, HIGH);
    }
    else{
      if(onTime < offTime){
        if(rtc.getHours() >= onTime && rtc.getHours() < offTime){
          tft.clearTextArea(ILI9341_GREEN);
          tft.setFontMode(gTextFontModeTransparent);
          tft.print("     ");
          tft.setFontMode(gTextFontModeSolid);
          tft.print("LIGHTS: ON");
          digitalWrite(relay3, LOW);
        }
        else{
          tft.clearTextArea(ILI9341_RED);
          tft.setFontMode(gTextFontModeTransparent);
          tft.print("     ");
          tft.setFontMode(gTextFontModeSolid);
          tft.print("LIGHTS: OFF");
          digitalWrite(relay3, HIGH);
        }
      }
      else{
        if(rtc.getHours() >= onTime && rtc.getHours() > offTime){
          tft.clearTextArea(ILI9341_GREEN);
          tft.setFontMode(gTextFontModeTransparent);
          tft.print("     ");
          tft.setFontMode(gTextFontModeSolid);
          tft.print("LIGHTS: ON");
          digitalWrite(relay3, LOW);
        }
        else if(rtc.getHours() <= onTime && rtc.getHours() < offTime){
          tft.clearTextArea(ILI9341_GREEN);
          tft.setFontMode(gTextFontModeTransparent);
          tft.print("     ");
          tft.setFontMode(gTextFontModeSolid);
          tft.print("LIGHTS: ON");
          digitalWrite(relay3, LOW);
        }
        else{
          tft.clearTextArea(ILI9341_RED);
          tft.setFontMode(gTextFontModeTransparent);
          tft.print("     ");
          tft.setFontMode(gTextFontModeSolid);
          tft.print("LIGHTS: OFF");
          digitalWrite(relay3, HIGH);
        }
      }
    }
    tft.setTextArea(0, 0, 320, 240);
}
