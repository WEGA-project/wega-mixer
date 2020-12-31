
// main code - don't change if you don't know what you are doing
////////////////////////////////////////////////////////////////
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
ESP8266WebServer server(80);

#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>


#include <Wire.h>
#include "src/Adafruit_MCP23017/Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;

// Pump #1
#define pump1 a0
#define pump1r b0
// Pump #2
#define pump2 a1
#define pump2r b1
// Pump #3
#define pump3 a2
#define pump3r b2
// Pump #4
#define pump4 a3
#define pump4r b3
// Pump #5
#define pump5 a4
#define pump5r b4
// Pump #6
#define pump6 a5
#define pump6r b5
// Pump #7
#define pump7 a6
#define pump7r b6
// Pump #8
#define pump8 a7
#define pump8r b7


#include "src/LiquidCrystal_I2C/LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27,16,2); // Check I2C address of LCD, normally 0x27 or 0x3F

#include "src/HX711/src/HX711.h"
const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;
HX711 scale;

//Коэффициенты фильтрации Кальмана
float Kl1= 0.1, Pr1=0.0001, Pc1=0.0, G1=1.0, P1=0.0, Xp1=0.0, Zp1=0.0, Xe1=0.0;

float p1,p2,p3,p4,p5,p6,p7,p8;

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  MDNS.begin("mixer");
  MDNS.addService("http", "tcp", 80);
  server.on("/", handleRoot);
  server.on("/test", test);
  server.on("/st", st);
  server.begin();
  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();

    Wire.begin(D1, D2);
  //Wire.setClock(400000L);   // 400 kHz I2C clock speed
    lcd.begin(D1,D2);      // SDA=D1, SCL=D2               
    lcd.backlight();

scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//scale.set_scale(1738.f); //B side
scale.set_scale(scale_calibration); //A side
   lcd.setCursor(10, 0);
  lcd.print("Start"); 
scale.power_up();
  server.handleClient();

delay (3000);
scale.tare(255);
}


void handleRoot() {
 String message = "Plan<br>P1:";
       message += fFTS(p1,3);
       message += "<br>P2:";
       message += fFTS(p2,3);
       message += "<br>P3:";
       message += fFTS(p3,3);
       message += "<br>P4:"; 
       message += fFTS(p4,3);
       message += "<br>P5:";
       message += fFTS(p5,3);
       message += "<br>P6:";
       message += fFTS(p6,3);
       message += "<br>P7:";    
       message += fFTS(p7,3);
       message += "<br>P8:";
       message += fFTS(p8,3);

       message += "<br><br><a href=""plan1"">PLAN1 START</a>"; 

message += "<form action='st' method='get'>";
message += "<p>P1 = <input type='text' name='p1' value='" + server.arg("p1") + "'/>Ca(NO3)2</p>";
message += "<p>P2 = <input type='text' name='p2' value='" + server.arg("p2") + "'/>KNO3</p>";
message += "<p>P3 = <input type='text' name='p3' value='" + server.arg("p3") + "'/>NH4NO3</p>";
message += "<p>P4 = <input type='text' name='p4' value='" + server.arg("p4") + "'/>MgSO4</p>";
message += "<p>P5 = <input type='text' name='p5' value='" + server.arg("p5") + "'/>KH2PO4</p>";
message += "<p>P6 = <input type='text' name='p6' value='" + server.arg("p6") + "'/>K2SO4</p>";
message += "<p>P7 = <input type='text' name='p7' value='" + server.arg("p7") + "'/>Micro 1000:1";
message += "<p>P8 = <input type='text' name='p8' value='" + server.arg("p8") + "'/>B";

message += "<p><input type='submit' value='Start'/></p>";



          
  server.send(200, "text/html", message);
 scale.tare(255); 

   }





void test (){
    float dl=30000;
    server.send(200, "text/html", "testing pump...");
    lcd.home();lcd.print("Pump 1 Start");PumpStart(pump1,pump1r);delay(dl);lcd.home();lcd.print("Pump 1 Revers       ");PumpReverse(pump1,pump1r);delay(dl);lcd.home();lcd.print("Pump 1 Stop      ");delay(1000);PumpStop(pump1,pump1r);
    lcd.home();lcd.print("Pump 2 Start");PumpStart(pump2,pump2r);delay(dl);lcd.home();lcd.print("Pump 2 Revers       ");PumpReverse(pump2,pump2r);delay(dl);lcd.home();lcd.print("Pump 2 Stop      ");delay(1000);PumpStop(pump2,pump2r);
    lcd.home();lcd.print("Pump 3 Start");PumpStart(pump3,pump3r);delay(dl);lcd.home();lcd.print("Pump 3 Revers       ");PumpReverse(pump3,pump3r);delay(dl);lcd.home();lcd.print("Pump 3 Stop      ");delay(1000);PumpStop(pump3,pump3r);
    lcd.home();lcd.print("Pump 4 Start");PumpStart(pump4,pump4r);delay(dl);lcd.home();lcd.print("Pump 4 Revers       ");PumpReverse(pump4,pump4r);delay(dl);lcd.home();lcd.print("Pump 4 Stop      ");delay(1000);PumpStop(pump4,pump4r);
    lcd.home();lcd.print("Pump 5 Start");PumpStart(pump5,pump5r);delay(dl);lcd.home();lcd.print("Pump 5 Revers       ");PumpReverse(pump5,pump5r);delay(dl);lcd.home();lcd.print("Pump 5 Stop      ");delay(1000);PumpStop(pump5,pump5r);
    lcd.home();lcd.print("Pump 6 Start");PumpStart(pump6,pump6r);delay(dl);lcd.home();lcd.print("Pump 6 Revers       ");PumpReverse(pump6,pump6r);delay(dl);lcd.home();lcd.print("Pump 6 Stop      ");delay(1000);PumpStop(pump6,pump6r);
    lcd.home();lcd.print("Pump 7 Start");PumpStart(pump7,pump7r);delay(dl);lcd.home();lcd.print("Pump 7 Revers       ");PumpReverse(pump7,pump7r);delay(dl);lcd.home();lcd.print("Pump 7 Stop      ");delay(1000);PumpStop(pump7,pump7r);
    lcd.home();lcd.print("Pump 8 Start");PumpStart(pump8,pump8r);delay(dl);lcd.home();lcd.print("Pump 8 Revers       ");PumpReverse(pump8,pump8r);delay(dl);lcd.home();lcd.print("Pump 8 Stop      ");delay(1000);PumpStop(pump8,pump8r);
  }



void st() {

float v1=server.arg("p1").toFloat();
float v2=server.arg("p2").toFloat();
float v3=server.arg("p3").toFloat();
float v4=server.arg("p4").toFloat();
float v5=server.arg("p5").toFloat();
float v6=server.arg("p6").toFloat();
float v7=server.arg("p7").toFloat();
float v8=server.arg("p8").toFloat();

 String message = "Plant<br>P1:";
       message += fFTS(v1,2);
       message += "<br>P2:";
       message += fFTS(v2,2);
       message += "<br>P3:";
       message += fFTS(v3,2);
       message += "<br>P4:"; 
       message += fFTS(v4,2);
       message += "<br>P5:";
       message += fFTS(v5,2);
       message += "<br>P6:";
       message += fFTS(v6,2);
       message += "<br>P7:";    
       message += fFTS(v7,2);
       message += "<br>P8:";
       message += fFTS(v8,2);
          
  server.send(200, "text/html", message);
// A (1-3)
scale.set_scale(scale_calibration); //A side
  p1=pumping(v1, pump1,pump1r, "1:Ca", 4000);
  p2=pumping(v2, pump2,pump2r, "2:KN", 4000);
  p3=pumping(v3, pump3,pump3r, "3:NH", 3500);

 
// B (4-8)
scale.set_scale(1738.f); //B side  
  p4=pumping(v4, pump4,pump4r, "4:Mg", 4000);
  p5=pumping(v5, pump5,pump5r, "5:KP", 4000);
  p6=pumping(v6, pump6,pump6r, "6:KS", 4000);
  p7=pumping(v7, pump7,pump7r, "7:Mk", 4000);
  p8=pumping(v8, pump8,pump8r, "8:B", 4000); 

WiFiClient client;
HTTPClient http;
String httpstr="http://192.168.237.107/remote/mixerdb.php?";
httpstr +=  "p1=" + fFTS(p1,3);
httpstr +=  "&p2=" + fFTS(p2,3);
httpstr +=  "&p3=" + fFTS(p3,3);
httpstr +=  "&p4=" + fFTS(p4,3);
httpstr +=  "&p5=" + fFTS(p5,3);
httpstr +=  "&p6=" + fFTS(p6,3);
httpstr +=  "&p7=" + fFTS(p7,3);
httpstr +=  "&p8=" + fFTS(p8,3);

httpstr +=  "&v1=" + fFTS(v1,3);
httpstr +=  "&v2=" + fFTS(v2,3);
httpstr +=  "&v3=" + fFTS(v3,3);
httpstr +=  "&v4=" + fFTS(v4,3);
httpstr +=  "&v5=" + fFTS(v5,3);
httpstr +=  "&v6=" + fFTS(v6,3);
httpstr +=  "&v7=" + fFTS(v7,3);
httpstr +=  "&v8=" + fFTS(v8,3);


http.begin(client, httpstr);
http.GET();
http.end();


}





void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  
 lcd.setCursor(0, 1);
 float scl=scale.get_units(16);
 float fscl=fl1(scl);
 if (abs(scl-fscl)/fscl > 0.05) {Xe1=scl;} 
 lcd.print(fscl,2);
 lcd.print("         ");
   
  lcd.setCursor(10, 0);
  lcd.print("Ready  "); 
}

// Функция преобразования чисел с плавающей запятой в текст  
String fFTS(float x, byte precision) {
  char tmp[50];
  dtostrf(x, 0, precision, tmp);
  return String(tmp);
}

//функция фильтрации Кальмана 1
float fl1(float val) { 
  Pc1 = P1 + Pr1;
  G1 = Pc1/(Pc1 + Kl1);
  P1 = (1-G1)*Pc1;
  Xp1 = Xe1;
  Zp1 = Xp1;
  Xe1 = G1*(val-Zp1)+Xp1; // "фильтрованное" значение
  return(Xe1);
}
// Функции помп

float PumpStart(int npump,int npumpr) {
  mcp.begin();
  mcp.pinMode(npump, OUTPUT); mcp.pinMode(npumpr, OUTPUT);
  mcp.digitalWrite(npump, HIGH);mcp.digitalWrite(npumpr, LOW);
  }
float PumpStop(int npump,int npumpr) {
  mcp.begin();
  mcp.pinMode(npump, OUTPUT); mcp.pinMode(npumpr, OUTPUT);
  mcp.digitalWrite(npump, LOW);mcp.digitalWrite(npumpr, LOW);
  }
float PumpReverse(int npump,int npumpr) {
  mcp.begin();
  mcp.pinMode(npump, OUTPUT); mcp.pinMode(npumpr, OUTPUT);
  mcp.digitalWrite(npump, LOW);mcp.digitalWrite(npumpr, HIGH);
  }
  
//Функция налива
float pumping(float wt, int npump,int npumpr, String nm, int preload) {


if (wt != 0 and wt < 400){
  //Продувка
  lcd.clear();  lcd.setCursor(0, 0); lcd.print(nm);lcd.print(" Reverse...");
PumpReverse(npump,npumpr);
delay(30000);

lcd.clear();  lcd.setCursor(0, 0); lcd.print(nm);
              lcd.setCursor(0, 1);lcd.print(" Preload=");lcd.print(preload);lcd.print("ms");
  scale.tare(255);
  mcp.begin();
  
PumpStart(npump,npumpr);
delay(preload);
PumpStop(npump,npumpr);


  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);

  lcd.setCursor(10, 0);
  lcd.print("RUNING");
  float value=scale.get_units(64);
  float pvalue,sk;
  while ( value < wt-0.01 ) {
    lcd.setCursor(0, 1);
    lcd.print(value,2);
    lcd.print(" (");
    lcd.print(100-(wt-value)/wt*100,0);
    lcd.print("%) ");
    lcd.print(sk,0);
    lcd.print("ms     ");
    //mcp.pinMode(npump, OUTPUT);
    //mcp.digitalWrite(npump, HIGH);
    PumpStart(npump,npumpr);
    if (value < (wt-1.5)) { 
      if (wt - value > 20) {delay (10000);}else{delay (2000);}
      pvalue=value;
      sk=80;
      }
      else {
          lcd.setCursor(10, 0);
          lcd.print("PCIS ");

        if (value-pvalue < 0.01) {if (sk<80){sk=sk+2;}}
        if (value-pvalue > 0.01) {if (sk>2) {sk=sk-2;}}
        if (value-pvalue > 0.1 ) {sk=0;}

        pvalue=value;
        delay (sk);
              
        }
    //mcp.digitalWrite(npump, LOW);
    PumpStop(npump,npumpr);
    delay (100);

    value=scale.get_units(254);
    Xe1=value;

    }
  //mcp.digitalWrite(npump, LOW);
   PumpStop(npump,npumpr);
    lcd.setCursor(0, 1);
    lcd.print(value,2);
    lcd.print(" (");
    lcd.print(100-(wt-value)/wt*100,2);
    lcd.print("%)      ");
      server.handleClient();
PumpReverse(npump,npumpr);
   delay (10000);
PumpStop(npump,npumpr);
    return value;
  }
else {
  lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);
  lcd.setCursor(10, 0);
  lcd.print("SKIP..   ");
  delay (1000);}
}
