///////////////////////////////////////////////////////////////////
// main code - don't change if you don't know what you are doing //
///////////////////////////////////////////////////////////////////
#define FW_version  "1.050"


#ifdef board_ESP32
#define board "ESP32"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
WebServer server(80);
#else
#define board "ESP8266"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
ESP8266WebServer server(80)
#endif

#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>

#include <Wire.h>
#include "src/Adafruit_MCP23017/Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;
// Assign ports names
// Here is the naming convention:
// A0=0 .. A7=7
// B0=8 .. B7=15
// That means if you have A0 == 0 B0 == 8 (this is how it's on the board B0/A0 == 8/0)
// one more example B3/A3 == 11/3
#define A0 0
#define A1 1
#define A2 2
#define A3 3
#define A4 4
#define A5 5
#define A6 6
#define A7 7
#define B0 8
#define B1 9
#define B2 10
#define B3 11
#define B4 12
#define B5 13
#define B6 14
#define B7 15

#ifdef board_ESP32
#define D1 19
#define D2 18
#define D5 16
#define D6 17
#endif

#include "src/LiquidCrystal_I2C/LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27,16,2); // Check I2C address of LCD, normally 0x27 or 0x3F

#include "src/HX711/src/HX711.h"
const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;
HX711 scale;

//Коэффициенты фильтрации Кальмана
float Kl1=0.1, Pr1=0.0001, Pc1=0.0, G1=1.0, P1=0.0, Xp1=0.0, Zp1=0.0, Xe1=0.0;

float p1,p2,p3,p4,p5,p6,p7,p8,fscl,curvol;
float RawStartA,RawEndA,RawStartB,RawEndB;
String wstatus,wpomp;
float mTimes, eTimes;

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  MDNS.begin("mixer");
  MDNS.addService("http", "tcp", 80);
  server.on("/", handleRoot);
  server.on("/test", test);
  server.on("/scales", scales);
  server.on("/st", st);
  server.on("/calibrate", calibrate);
  server.on("/tare", tare);
  server.on("/style.css", css);
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
scale.set_scale(scale_calibration_A); //A side
 lcd.setCursor(0, 0);
  lcd.print("Start FW: ");
  lcd.print(FW_version);
 lcd.setCursor(0, 1); 
  lcd.print(WiFi.localIP()); 
scale.power_up();

wstatus="Ready";

  server.handleClient();

delay (3000);
tareScalesWithCheck(255);
lcd.clear();
}


void handleRoot() {
 String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
 
        message += "Status: " + wstatus + "<br>";
        message += "<br>Sum A = " + fFTS( (rawToUnits(RawEndA)-rawToUnits(RawStartA)) ,2);
        message += "<br>Sum B = " + fFTS( (rawToUnits(RawEndB)-rawToUnits(RawStartB)) ,2);
        message += "<br>Timer = " + fFTS( eTimes/1000 ,0) + " sec";
        message += "<br>";
    
 if (wstatus != "Ready" )
  {message = " Work pomp: " + wpomp + "<br>Current vol: " + fFTS(curvol,2) + "g";
   message += "<br>Timer = " + fFTS( (millis()-mTimes)/1000 ,0) + " sec";
  }

float p1f=server.arg("p1").toFloat(), p1c=(p1-p1f)/p1f*100;
float p2f=server.arg("p2").toFloat(), p2c=(p2-p2f)/p2f*100;
float p3f=server.arg("p3").toFloat(), p3c=(p3-p3f)/p3f*100;
float p4f=server.arg("p4").toFloat(), p4c=(p4-p4f)/p4f*100;
float p5f=server.arg("p5").toFloat(), p5c=(p5-p5f)/p5f*100;
float p6f=server.arg("p6").toFloat(), p6c=(p6-p6f)/p6f*100;
float p7f=server.arg("p7").toFloat(), p7c=(p7-p7f)/p7f*100;
float p8f=server.arg("p8").toFloat(), p8c=(p8-p8f)/p8f*100;

String prc1,prc2,prc3,prc4,prc5,prc6,prc7,prc8;
if ( p1f != NULL and p1f != 0){ prc1="="+fFTS(p1,2)+" "+fFTS(p1c,2)+"%";}else{prc1="";}
if ( p2f != NULL and p2f != 0){ prc2="="+fFTS(p2,2)+" "+fFTS(p2c,2)+"%";}else{prc2="";}
if ( p3f != NULL and p3f != 0){ prc3="="+fFTS(p3,2)+" "+fFTS(p3c,2)+"%";}else{prc3="";}
if ( p4f != NULL and p4f != 0){ prc4="="+fFTS(p4,2)+" "+fFTS(p4c,2)+"%";}else{prc4="";}
if ( p5f != NULL and p5f != 0){ prc5="="+fFTS(p5,2)+" "+fFTS(p5c,2)+"%";}else{prc5="";}
if ( p6f != NULL and p6f != 0){ prc6="="+fFTS(p6,2)+" "+fFTS(p6c,2)+"%";}else{prc6="";}
if ( p7f != NULL and p7f != 0){ prc7="="+fFTS(p7,2)+" "+fFTS(p7c,2)+"%";}else{prc7="";}
if ( p8f != NULL and p8f != 0){ prc8="="+fFTS(p8,2)+" "+fFTS(p8c,2)+"%";}else{prc8="";}

message += "<form action='st' method='get'>";
message += "<p>P1 = <input type='text' name='p1' value='" + server.arg("p1") + "'/> "+pump1n +prc1+"</p>";
message += "<p>P2 = <input type='text' name='p2' value='" + server.arg("p2") + "'/> "+pump2n +prc2+"</p>";
message += "<p>P3 = <input type='text' name='p3' value='" + server.arg("p3") + "'/> "+pump3n +prc3+"</p>";
message += "<p>P4 = <input type='text' name='p4' value='" + server.arg("p4") + "'/> "+pump4n +prc4+"</p>";
message += "<p>P5 = <input type='text' name='p5' value='" + server.arg("p5") + "'/> "+pump5n +prc5+"</p>";
message += "<p>P6 = <input type='text' name='p6' value='" + server.arg("p6") + "'/> "+pump6n +prc6+"</p>";
message += "<p>P7 = <input type='text' name='p7' value='" + server.arg("p7") + "'/> "+pump7n +prc7+"</p>";
message += "<p>P8 = <input type='text' name='p8' value='" + server.arg("p8") + "'/> "+pump8n +prc8+"</p>";

if (wstatus == "Ready" )  
 { 

   message += "<p><input type='submit' class='button' value='Start'/>  ";
   message += "<input type='button' class='button' onclick=\"window.location.href = 'scales';\" value='Scales'/>  ";
   message += "<input type='button' class='button' onclick=\"window.location.href = 'calibrate';\" value='Calibrate'/>";
   message += "</p></form>";
 }
else
 {
  message += "<meta http-equiv='refresh' content='10'>";
 }
          
  server.send(200, "text/html", message);

   }


void scales (){
 String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
        message += "<meta http-equiv='refresh' content='5'>";
        message += "<h3>Current weight = " + fFTS(fscl,2) + "</h3>";
        message += "RAW = " + fFTS(unitsToRaw(fscl),0);
        message += "<p><input type='button' class='button' onclick=\"window.location.href = 'tare';\" value='Set to ZERO'/>  ";
        message += "<input type='button' class='button' onclick=\"window.location.href = '/';\" value='Home'/>";
        message += "</p>";

  server.send(200, "text/html", message);

  }

void tare (){
tareScalesWithCheck(255);
String message = "<script language='JavaScript' type='text/javascript'>setTimeout('window.history.go(-1)',0);</script>";
       message += "<input type='button' class='button' onclick='history.back();' value='back'/>";

  server.send(200, "text/html", message);

  }

void calibrate (){
float raw = unitsToRaw(readScalesWithCheck(255));
String  message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
        message += "Calibrate (calculate scale_calibration value)";
        message += "<h1>Current RAW = " + fFTS(raw,0) + "</h1>";
        scale.set_scale(scale_calibration_A);
        message += "<br><h2>Current Value for point A = " + fFTS(rawToUnits(raw),2) + "g</h2>";
        scale.set_scale(scale_calibration_B);
        message += "<br><h2>Current Value for point B = " + fFTS(rawToUnits(raw),2) + "g</h2>";
        message += "<br>Current scale_calibration_A = " + fFTS(scale_calibration_A,4);
        message += "<br>Current scale_calibration_B = " + fFTS(scale_calibration_B,4);  
message += "<form action='' method='get'>";
message += "<p>RAW on Zero <input type='text' name='x1' value='" + server.arg("x1") + "'/></p>";
message += "<p>RAW value with load <input type='text' name='x2' value='" + server.arg("x2") + "'/></p>";
message += "<p>Value with load (gramm) <input type='text' name='s2' value='" + server.arg("s2") + "'/></p>";
message += "<p><input type='submit' class='button' value='Submit'/>  ";
message += "<input type='button' class='button' onclick=\"window.location.href = 'tare';\" value='Set to ZERO'/>  ";
message += "<button class='button'  onClick='window.location.reload();'>Refresh</button>  ";
message += "<input class='button' type='button' onclick=\"window.location.href = '/';\" value='Home'/>";
message += "</p>";

float x1=server.arg("x1").toFloat();
float x2=server.arg("x2").toFloat();
float s2=server.arg("s2").toFloat();
float k,y,s;

if (s2 != 0) 
 {
  k=-(x1-x2)/s2;
  message += "<br> scale_calibration = <b>"+fFTS(k,4)+"</b> copy and paste to your sketch";
 }

if (x1 > 0 and x2 > 0) 
 {
  y=-(s2*x1)/(x1-x2);
  message += "<br>Calculate preloaded weight = "+fFTS(y,2) + "g";
 }
 
if (s2 != 0)
 { 
  s=raw*(1/k)-y;
  message += "<br>Calculate weight = "+fFTS(s,2) + "g";
 }

  server.send(200, "text/html", message);  
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
mTimes=millis();

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


       message += "<meta http-equiv='refresh' content=0;URL=../";
       message += "?p1="+server.arg("p1");
       message += "&p2="+server.arg("p2");
       message += "&p3="+server.arg("p3");
       message += "&p4="+server.arg("p4");
       message += "&p5="+server.arg("p5");
       message += "&p6="+server.arg("p6");
       message += "&p7="+server.arg("p7");
       message += "&p8="+server.arg("p8");
       message += ">";
          
  server.send(200, "text/html", message);
// A (1-3)
scale.set_scale(scale_calibration_A); //A side
RawStartA=unitsToRaw(readScalesWithCheck(255));

  p1=pumping(v1, pump1,pump1r, pump1n, pump1p);
  p2=pumping(v2, pump2,pump2r, pump2n, pump2p);
  p3=pumping(v3, pump3,pump3r, pump3n, pump3p);
  
RawEndA=unitsToRaw(readScalesWithCheck(255));  
 
// B (4-8)
scale.set_scale(scale_calibration_B); //B side 
RawStartB=unitsToRaw(readScalesWithCheck(255)); 

  p4=pumping(v4, pump4,pump4r, pump4n, pump4p);
  p5=pumping(v5, pump5,pump5r, pump5n, pump5p);
  p6=pumping(v6, pump6,pump6r, pump6n, pump6p);
  p7=pumping(v7, pump7,pump7r, pump7n, pump7p);
  p8=pumping(v8, pump8,pump8r, pump8n, pump8p); 
  
RawEndB=unitsToRaw(readScalesWithCheck(255)); 
 
  wstatus="Ready";
  eTimes=millis()-mTimes;
  
WiFiClient client;
HTTPClient http;
String httpstr=WegaApiUrl;
httpstr +=  "?p1=" + fFTS(p1,3);
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

delay (10000);
lcd.clear();
}


void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  #if (KALMAN || !defined(KALMAN))
    float scl  = scale.get_units(16);
    fscl = kalmanFilter(scl);
    if (abs(scl-fscl)/fscl > 0.05) {Xe1=scl;}
  #else
    fscl = scale.get_units(128);  
  #endif
  lcd.setCursor(0, 1);
  lcd.print(fscl, 2);
  lcd.print("         ");
  
  lcd.setCursor(10, 0);
  lcd.print("Ready  "); 
}

// Функция преобразования чисел с плавающей запятой в текст
// Function: convert float numbers to string
String fFTS(float x, byte precision) {
  char tmp[50];
  dtostrf(x, 0, precision, tmp);
  return String(tmp);
}

float kalmanFilter(float val) { 
  Pc1 = P1 + Pr1;
  G1 = Pc1/(Pc1 + Kl1);
  P1 = (1-G1)*Pc1;
  Xp1 = Xe1;
  Zp1 = Xp1;
  Xe1 = G1*(val-Zp1)+Xp1; // "фильтрованное" значение - Filtered value
  return(Xe1);
}

// Функции помп
// Pump function
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

// Функция налива
// Function: pour solution
float pumping(float wt, int npump,int npumpr, String nm, int preload) {
wstatus="Worked...";
wpomp=nm;

  server.handleClient();

if (wt != 0 and wt < 400){
  // Продувка
  // Mix the solution
  lcd.clear();  lcd.setCursor(0, 0); lcd.print(nm);lcd.print(" Reverse...");
PumpReverse(npump,npumpr);
delay(preload);

lcd.clear();  lcd.setCursor(0, 0); lcd.print(nm);
              lcd.setCursor(0, 1);lcd.print(" Preload=");lcd.print(preload);lcd.print("ms");
  tareScalesWithCheck(255);
  mcp.begin();
  
PumpStart(npump,npumpr);

delay(preload);

      server.handleClient();
      ArduinoOTA.handle();
PumpStop(npump,npumpr);


  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);

  lcd.setCursor(10, 0);
  lcd.print("RUNING");


  float value = readScalesWithCheck(128);
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
    
      
      if (wt - value > 20) {delay (10000);}else{delay (1000);}
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

    server.handleClient();
    ArduinoOTA.handle(); 
    
    delay (100);

    value=readScalesWithCheck(128);
    Xe1=value;
    curvol=value;
    }
   PumpStop(npump,npumpr);
    lcd.setCursor(0, 1);
    lcd.print(value,2);
    lcd.print(" (");
    lcd.print(100-(wt-value)/wt*100,2);
    lcd.print("%)      ");
      //server.handleClient();
PumpReverse(npump,npumpr);
   delay (preload*2);
PumpStop(npump,npumpr);
    
    return value;
  }
else {
  lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);
  lcd.setCursor(10, 0);
  lcd.print("SKIP..   ");
  delay (1000);
  server.handleClient();
  }
}


// Функции для работы с весами
float readScales(int times) {
  float value1 = scale.get_units(times / 2);
  delay(20);
  float value2 = scale.get_units(times / 2);
  return (fabs(value1 - value2) > 0.01) ? NAN: (value1 + value2) / 2;
}

float readScalesWithCheck(int times) {
  while (true) {
    float result = readScales(times);
    if (!isnan(result)) {
      return result;
    }
  }
}

void tareScalesWithCheck(int times) {
  scale.set_offset(unitsToRaw(readScalesWithCheck(times)));
}

float unitsToRaw(float units) {
  return units * scale.get_scale() + scale.get_offset();
}

float rawToUnits(float raw) {
  return (raw - scale.get_offset()) / scale.get_scale();
}
