///////////////////////////////////////////////////////////////////
// main code - don't change if you don't know what you are doing //
///////////////////////////////////////////////////////////////////
#define FW_version  "2.0.1 igor"

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

#include "src/LiquidCrystal_I2C/LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27,16,2); // Check I2C address of LCD, normally 0x27 or 0x3F

#include "src/HX711/src/HX711.h"
const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;
HX711 scale;

class Kalman { // https://github.com/denyssene/SimpleKalmanFilter
  private:
  float err_measure, err_estimate, q, last_estimate;  

  public:
  Kalman(float _err_measure, float _err_estimate, float _q) {
      err_measure = _err_measure;
      err_estimate = _err_estimate;
      q = _q;
  }

  float updateEstimation(float measurement) {
    float gain = err_estimate / (err_estimate + err_measure);
    float current_estimate = last_estimate + gain * (measurement - last_estimate);
    err_estimate =  (1.0 - gain) * err_estimate + fabs(last_estimate - current_estimate) * q;
    last_estimate = current_estimate;
    return last_estimate;
  }
  float getEstimation() {
    return last_estimate;
  }
};

Kalman displayFilter = Kalman(1400, 80, 0.15); // плавный
Kalman filter = Kalman(1000, 80, 0.4);         // резкий

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
        message += "<br>Sum A = " + toString(((RawEndA - RawStartA) / scale_calibration_A) ,2);
        message += "<br>Sum B = " + toString(((RawEndB - RawStartB) / scale_calibration_B) ,2);
        message += "<br>Timer = " + toString( eTimes/1000 ,0) + " sec";
        message += "<br>";
    
  if (wstatus != "Ready" ) {
    message = " Work pomp: " + wpomp + "<br>Current vol: " + toString(curvol,2) + "g";
    message += "<br>Timer = " + toString( (millis()-mTimes)/1000 ,0) + " sec";
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
  if ( p1f != NULL and p1f != 0){ prc1="="+toString(p1,2)+" "+toString(p1c,2)+"%";}else{prc1="";}
  if ( p2f != NULL and p2f != 0){ prc2="="+toString(p2,2)+" "+toString(p2c,2)+"%";}else{prc2="";}
  if ( p3f != NULL and p3f != 0){ prc3="="+toString(p3,2)+" "+toString(p3c,2)+"%";}else{prc3="";}
  if ( p4f != NULL and p4f != 0){ prc4="="+toString(p4,2)+" "+toString(p4c,2)+"%";}else{prc4="";}
  if ( p5f != NULL and p5f != 0){ prc5="="+toString(p5,2)+" "+toString(p5c,2)+"%";}else{prc5="";}
  if ( p6f != NULL and p6f != 0){ prc6="="+toString(p6,2)+" "+toString(p6c,2)+"%";}else{prc6="";}
  if ( p7f != NULL and p7f != 0){ prc7="="+toString(p7,2)+" "+toString(p7c,2)+"%";}else{prc7="";}
  if ( p8f != NULL and p8f != 0){ prc8="="+toString(p8,2)+" "+toString(p8c,2)+"%";}else{prc8="";}
  
  message += "<form action='st' method='get'>";
  message += "<p>P1 = <input type='text' name='p1' value='" + server.arg("p1") + "'/> "+pump1n +prc1+"</p>";
  message += "<p>P2 = <input type='text' name='p2' value='" + server.arg("p2") + "'/> "+pump2n +prc2+"</p>";
  message += "<p>P3 = <input type='text' name='p3' value='" + server.arg("p3") + "'/> "+pump3n +prc3+"</p>";
  message += "<p>P4 = <input type='text' name='p4' value='" + server.arg("p4") + "'/> "+pump4n +prc4+"</p>";
  message += "<p>P5 = <input type='text' name='p5' value='" + server.arg("p5") + "'/> "+pump5n +prc5+"</p>";
  message += "<p>P6 = <input type='text' name='p6' value='" + server.arg("p6") + "'/> "+pump6n +prc6+"</p>";
  message += "<p>P7 = <input type='text' name='p7' value='" + server.arg("p7") + "'/> "+pump7n +prc7+"</p>";
  message += "<p>P8 = <input type='text' name='p8' value='" + server.arg("p8") + "'/> "+pump8n +prc8+"</p>";
  
  if (wstatus == "Ready" ) { 
     message += "<p><input type='submit' class='button' value='Start'/>  ";
     message += "<input type='button' class='button' onclick=\"window.location.href = 'scales';\" value='Scales'/>  ";
     message += "<input type='button' class='button' onclick=\"window.location.href = 'calibrate';\" value='Calibrate'/>";
     message += "</p></form>";
  } else {
    message += "<meta http-equiv='refresh' content='10'>";
  }
          
  server.send(200, "text/html", message);
}

void scales (){
 String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
        message += "<meta http-equiv='refresh' content='5'>";
        message += "<h3>Current weight = " + toString(rawToUnits(fscl),2) + "</h3>";
        message += "RAW = " + toString(fscl,0);
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
  float raw = readScalesWithCheck(255);
  String  message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
          message += "Calibrate (calculate scale_calibration value)";
          message += "<h1>Current RAW = " + toString(raw,0) + "</h1>";
          message += "<br><h2>Current Value for point A = " + toString(rawToUnits(raw, scale_calibration_A),2) + "g</h2>";
          message += "<br><h2>Current Value for point B = " + toString(rawToUnits(raw, scale_calibration_B),2) + "g</h2>";
          message += "<br>Current scale_calibration_A = " + toString(scale_calibration_A,4);
          message += "<br>Current scale_calibration_B = " + toString(scale_calibration_B,4);  
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
  
  if (s2 != 0) {
    k=-(x1-x2)/s2;
    message += "<br> scale_calibration = <b>"+toString(k,4)+"</b> copy and paste to your sketch";
  }
  
  if (x1 > 0 and x2 > 0) {
    y=-(s2*x1)/(x1-x2);
    message += "<br>Calculate preloaded weight = "+toString(y,2) + "g";
  }
   
  if (s2 != 0) { 
    s=raw*(1/k)-y;
    message += "<br>Calculate weight = "+toString(s,2) + "g";
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
         message += toString(v1,2);
         message += "<br>P2:";
         message += toString(v2,2);
         message += "<br>P3:";
         message += toString(v3,2);
         message += "<br>P4:"; 
         message += toString(v4,2);
         message += "<br>P5:";
         message += toString(v5,2);
         message += "<br>P6:";
         message += toString(v6,2);
         message += "<br>P7:";    
         message += toString(v7,2);
         message += "<br>P8:";
         message += toString(v8,2);
  
  
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

  float offsetBeforePump = scale.get_offset();
  
  // A (1-3)
  scale.set_scale(scale_calibration_A); //A side
  RawStartA=readScalesWithCheck(255);
  p1=pumping(v1, pump1,pump1r, pump1n, pump1p);
  p2=pumping(v2, pump2,pump2r, pump2n, pump2p);
  p3=pumping(v3, pump3,pump3r, pump3n, pump3p);
  RawEndA=readScalesWithCheck(255);   
  
  // B (4-8)
  scale.set_scale(scale_calibration_B); //B side 
  RawStartB=RawEndA; 
  p4=pumping(v4, pump4,pump4r, pump4n, pump4p);
  p5=pumping(v5, pump5,pump5r, pump5n, pump5p);
  p6=pumping(v6, pump6,pump6r, pump6n, pump6p);
  p7=pumping(v7, pump7,pump7r, pump7n, pump7p);
  p8=pumping(v8, pump8,pump8r, pump8n, pump8p); 
  RawEndB=readScalesWithCheck(255); 
  
  scale.set_offset(offsetBeforePump);
  wstatus="Ready";
  eTimes=millis()-mTimes;
  
  WiFiClient client;
  HTTPClient http;
  String httpstr=WegaApiUrl;
  httpstr +=  "?p1=" + toString(p1,3);
  httpstr +=  "&p2=" + toString(p2,3);
  httpstr +=  "&p3=" + toString(p3,3);
  httpstr +=  "&p4=" + toString(p4,3);
  httpstr +=  "&p5=" + toString(p5,3);
  httpstr +=  "&p6=" + toString(p6,3);
  httpstr +=  "&p7=" + toString(p7,3);
  httpstr +=  "&p8=" + toString(p8,3);
  
  httpstr +=  "&v1=" + toString(v1,3);
  httpstr +=  "&v2=" + toString(v2,3);
  httpstr +=  "&v3=" + toString(v3,3);
  httpstr +=  "&v4=" + toString(v4,3);
  httpstr +=  "&v5=" + toString(v5,3);
  httpstr +=  "&v6=" + toString(v6,3);
  httpstr +=  "&v7=" + toString(v7,3);
  httpstr +=  "&v8=" + toString(v8,3);
  
  
  http.begin(client, httpstr);
  http.GET();
  http.end();
  
  delay (10000);
  lcd.clear();
}


void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  scale.set_scale(scale_calibration_A);
  delay(0);  
  #if (KALMAN || !defined(KALMAN))
    readScales(16);
    fscl = displayFilter.getEstimation();
  #else
    fscl = readScales(128);  
  #endif
  lcd.setCursor(0, 1);
  lcd.print(toString(rawToUnits(fscl), 2));
  lcd.print("         ");
  
  lcd.setCursor(10, 0);
  lcd.print("Ready  "); 
}

String toString(float x, byte precision) {
  if (x < 0 && -x < pow(0.1, precision)) {
    x = 0;
  }

  char tmp[50];
  dtostrf(x, 0, precision, tmp);
  return String(tmp);
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

void printValueAndPercent(float value, float targetValue) {
    lcd.setCursor(0, 1); 
    lcd.print(toString(value, 2)); 
    if (!isnan(targetValue)) {
      lcd.print(" ("); lcd.print(toString(value/targetValue*100,1)); lcd.print("%)");
    }
    lcd.print("    ");
    yield();
}

void pumpToValue(float capValue, float capMillis, float targetValue, int npump,int npumpr, float allowedMeasurementError) {
  float value = rawToUnits(filter.getEstimation());
  if (value >= capValue) {
    return;
  }
  long endMillis = millis() + capMillis;
  float maxValue = value;
  PumpStart(npump,npumpr);
  char exitCode;
  long i = 0;
  while (true) { 
    if (value >= capValue) {
      exitCode = 'V'; // вес достиг заданный
      break;
    } else if (millis() >= endMillis) {
      exitCode = 'T'; // истекло время
      break;
    } else if (value < maxValue - allowedMeasurementError) {
      exitCode = 'E'; // аномальные показания весов
      break;
    }
    server.handleClient();
    delay(2);
    readScales(1);
    value = rawToUnits(filter.getEstimation());
    maxValue = max(value, maxValue);
    if (i % 5 == 0) printValueAndPercent(value, targetValue);
  }
  PumpStop(npump,npumpr);
  printValueAndPercent(value, targetValue);
  lcd.setCursor(15, 1);lcd.print(exitCode);
}


// Функция налива
// Function: pour solution
float pumping(float wt, int npump,int npumpr, String nm, int staticPreload) {
  wstatus="Worked...";
  wpomp=nm;

  server.handleClient();

  if (wt <= 0) {
    lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);
    lcd.setCursor(10, 0);
    lcd.print("SKIP..   ");
    server.handleClient();
    delay (1000);
    return 0;
  }

  lcd.clear(); lcd.setCursor(0, 0); lcd.print(nm);lcd.print(" Tare...");
  float value = 0;
  tareScalesWithCheck(255);
  server.handleClient();
  delay(10);

  int preload;
  if (wt < 0.5) { // статический прелоад
    preload = staticPreload;
    lcd.clear(); lcd.setCursor(0, 0); lcd.print(nm);lcd.print(" Reverse...");
    PumpReverse(npump,npumpr);
    delay(preload);
    PumpStop(npump,npumpr);
    server.handleClient();
    delay(10);
  
    lcd.clear(); lcd.setCursor(0, 0); lcd.print(nm);lcd.print(" Preload...");
    lcd.setCursor(0, 1);lcd.print(" Preload=");lcd.print(preload);lcd.print("ms");
    PumpStart(npump,npumpr);
    delay(preload);
    PumpStop(npump,npumpr);
    server.handleClient();
    delay(10);

    value = rawToUnits(readScalesWithCheck(128));
    server.handleClient();
    delay(10);
  } else { // прелоад до первой капли
    lcd.clear(); lcd.setCursor(0, 0); lcd.print(nm);lcd.print(" Preload...");
    preload = 0;
    while (value < 0.02) {
      long startTime = millis();
      pumpToValue(0.03, staticPreload * 2, NAN, npump, npumpr, 0.1);
      preload += millis() - startTime;
      value = rawToUnits(readScalesWithCheck(128));
      curvol=value;
      server.handleClient();
      delay(10);
    }
    lcd.setCursor(0, 1);lcd.print(" Preload=");lcd.print(preload);lcd.print("ms");
    delay(1000);
  }
  
  // до конечного веса минус 0.2 - 0.5 грамм по половине от остатка 
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);lcd.print(" Fast...");
  float performance = 0.0007;
  float dropTreshold = wt - value > 1.0 ? 0.2 : 0.5; // определяет сколько оставить на капельный налив
  float valueToPump = wt - dropTreshold - value;
  float allowedOscillation = valueToPump < 1.0 ? 1.5 : 3.0;  // допустимое раскачивание, чтобы остановитmся при помехах/раскачивании, важно на малых объемах  
  if (valueToPump > 0.3) { // если быстро качать не много то не начинать даже
    while ((valueToPump = wt - dropTreshold - value) > 0) {
      if (valueToPump > 0.2) valueToPump = valueToPump / 2;  // качать по половине от остатка
      long timeToPump = valueToPump / performance;           // ограничение по времени
      long startTime = millis();
      pumpToValue(curvol + valueToPump, timeToPump, wt, npump, npumpr, allowedOscillation);
      long endTime = millis();
      value = rawToUnits(readScalesWithCheck(128));
      if (endTime - startTime > 200 && value-curvol > 0.15) performance = max(performance, (value - curvol) / (endTime - startTime));
      curvol=value;
      server.handleClient();
      delay(10);
    }
  }
  
  // капельный налив
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(nm);lcd.print(":");lcd.print(wt);lcd.print(" Drop...");
  float prevValue = value;
  int sk = 25;
  while (value < wt - 0.01) {
    lcd.setCursor(0, 1);
    lcd.print(value, 2);
    lcd.print(" (");
    lcd.print(value / wt * 100, 0);
    lcd.print("%) ");
    lcd.print(sk);
    lcd.print("ms     ");
      
    if (value - prevValue < 0.01) {sk = min(80, sk+2);}
    if (value - prevValue > 0.01) {sk = max(2, sk-2);}
    if (value - prevValue > 0.1 ) {sk = 0;}

    prevValue = value;
    PumpStart(npump,npumpr);
    delay(sk);
    PumpStop(npump,npumpr);

    server.handleClient();
    delay(100);
    value = rawToUnits(readScalesWithCheck(128));
    curvol = value;
  }

  lcd.setCursor(0, 1);
  lcd.print(toString(value, 2));
  lcd.print(" (");
  lcd.print(value/wt*100, 2);
  lcd.print("%)      ");

  // реверс, высушить трубки
  PumpReverse(npump, npumpr);
  long endPreloadTime = millis() + max(preload, staticPreload) * 1.5; 
  while (millis() < endPreloadTime) {
      server.handleClient();
      delay(10);
  }
  PumpStop(npump, npumpr);
    
  return value;
}

// Функции для работы с весами
float readScales(int times) {
  float sum = 0;
  for (int i = 0; i < times; i++) {
    float value = scale.read();
    sum += value;
    displayFilter.updateEstimation(value);
    filter.updateEstimation(value);
  }
  return sum / times;
}

float readScalesWithCheck(int times) {
  float value1 = readScales(times / 2);
  while (true) {
    delay(20);
    float value2 = readScales(times / 2);
    if (fabs(value1 - value2) < fabs(0.01 * scale_calibration_A)) {
      return (value1 + value2) / 2;
    }
    value1 = value2;
  }
}

void tareScalesWithCheck(int times) {
  scale.set_offset(readScalesWithCheck(times));
}

float rawToUnits(float raw) {
  return (raw - scale.get_offset()) / scale.get_scale();
}

float rawToUnits(float raw, float calibrationPoint) {
  return (raw - scale.get_offset()) / calibrationPoint;
}
