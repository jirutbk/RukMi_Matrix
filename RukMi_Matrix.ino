#include <ESP8266WiFi.h>    //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <WiFiManager.h>     //https://github.com/tzapu/WiFiManager 
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <EEPROM.h>
#include "mainPage.h"

#define led  D0
#define MAX_DEVICES 2
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D4
#define ESP_AP_NAME "RukMi_Matrix Config"
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)

MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
ESP8266WebServer server(80);
Ticker ticker;

uint8_t scrollSpeed = 100;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds
int eepromAddr = 60; //ตำแหน่งอ่าน/เขียน eeprom

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  120
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "RukMi Matrix V 1.0 " };
bool newMessageAvailable = true;

void setup() 
{  
  pinMode(led, OUTPUT);      
  ticker.attach(0.6, tick);  
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);   
  WiFiManager wifiManager;
  wifiManager.setTimeout(300); // กำหนด timeout ของโหมด AP 
  wifiManager.setAPCallback(configModeCallback); // กำหนด callback ของ AP ไปที่ configModeCallback
    
  if (!wifiManager.autoConnect(ESP_AP_NAME)) { 
    Serial.println("failed to connect and hit timeout");
    ESP.reset(); // reset อุปกรณ์
    delay(1000);
  }
  
  ticker.detach();
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(led,LOW);

  P.begin();
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
  
  EEPROM.begin(512);   
  eepRead();
    
  // เริ่มการทำงานของ Server
  server.on("/", handleRoot); 
  server.on("/cmd", handleCMD);  
  server.begin();
  Serial.println("Server started");
  
}

void eepWrite()
{    
  for(byte i=0; i< sizeof(newMessage); i++)
    EEPROM.write(eepromAddr+i, newMessage[i]);
 
  EEPROM.commit();
  newMessageAvailable = true;
}

void eepRead()
{ 
  if(EEPROM.read(eepromAddr) == 0xff) //ถ้ายังไม่เคยบันทึกใน eeprom ให้ข้าม
    return;  
  
  for(byte i=0; i< BUF_SIZE; i++)
  {    
    newMessage[i] = EEPROM.read(eepromAddr+i);
    Serial.print(newMessage[i],HEX);
  } 
  newMessageAvailable = true;
}

void tick() { 
  int state = digitalRead(led); 
  digitalWrite(led, !state); 
}

void configModeCallback (WiFiManager *myWiFiManager) { // callback เมื่อเชื่อม access point ไม่สำเร็จจะเข้าสู่โหมด AP
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick); 
}

void handleRoot(){  
  server.send(200, "text/html", MAIN_page); 
}

void handleCMD()  
{
  String text = server.arg("message");
  text.replace('\r' , ' ');  //ตัด \r ออก
  text.replace('\n' , ' ');  //ตัด \n ออก
  text.toCharArray(newMessage,BUF_SIZE);
  eepWrite();
  
  server.send(200, "text/html", "<html><head><meta charset='utf-8'></head><body><center><h2>OK : บันทึกข้อความเรียบร้อย</h2></center></body></html>"); 
}

void loop() {  
  server.handleClient(); 

  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  } 
}
