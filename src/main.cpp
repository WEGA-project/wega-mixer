///////////////////////////////////////////////////////////////////
// main code - don't change if you don't know what you are doing //
///////////////////////////////////////////////////////////////////

#define FW_version "ESPUNIV 1.1.a"

#define c_ESP 8266 // 8266 or 32


#if c_ESP == 32
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #include <HTTPClient.h>
  

  WebServer server(80);
#endif // c_ESP32

#if c_ESP == 8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266HTTPClient.h>

  ESP8266WebServer server(80);
#endif // c_ESP8266

  #include <WiFiClient.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
  

  
  



// #if c_ESP == 8266

//   #include <WiFiClient.h>
//   
//   
//   #include <WiFiUdp.h>
//   #include <ArduinoOTA.h>
//   ESP8266WebServer server(80);

//   #include <WiFiClient.h>
//   
// #endif // c_ESP8266

#include <Wire.h>
#include <Adafruit_MCP23017.h>
Adafruit_MCP23017 mcp;
// Assign ports names
// Here is the naming convention:
// A0=0 .. A7=7
// B0=8 .. B7=15
// That means if you have A0 == 0 B0 == 8 (this is how it's on the board B0/A0 == 8/0)
// one more example B3/A3 == 11/3
#define DRV1_A 0
#define DRV1_B 1
#define DRV1_C 2
#define DRV1_D 3
#define DRV2_A 4
#define DRV2_B 5
#define DRV2_C 6
#define DRV2_D 7
#define DRV3_A 8
#define DRV3_B 9
#define DRV3_C 10
#define DRV3_D 11
#define DRV4_A 12
#define DRV4_B 13
#define DRV4_C 14
#define DRV4_D 15

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F

#include <HX711.h>
const int LOADCELL_DOUT_PIN = 13;
const int LOADCELL_SCK_PIN = 14;
HX711 scale;

//Коэффициенты фильтрации Кальмана
float Kl1 = 0.1, Pr1 = 0.0001, Pc1 = 0.0, G1 = 1.0, P1 = 0.0, Xp1 = 0.0, Zp1 = 0.0, Xe1 = 0.0;

float p1, p2, p3, p4, p5, p6, p7, p8, fscl, curvol;
float RawStartA, RawEndA, RawStartB, RawEndB;
String wstatus, wpomp;
float mTimes, eTimes;

#include "pre.h"
#include "style.h"
#include "func.h"





void handleRoot()
{
  String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";

  message += "Status: " + wstatus + "<br>";
  message += "<br>Sum A = " + fFTS((rawToUnits(RawEndA) - rawToUnits(RawStartA)), 2);
  message += "<br>Sum B = " + fFTS((rawToUnits(RawEndB) - rawToUnits(RawStartB)), 2);
  message += "<br>Timer = " + fFTS(eTimes / 1000, 0) + " sec";
  message += "<br>";

  if (wstatus != "Ready")
  {
    message = " Work pomp: " + wpomp + "<br>Current vol: " + fFTS(curvol, 2) + "g";
    message += "<br>Timer = " + fFTS((millis() - mTimes) / 1000, 0) + " sec";
  }

  float p1f = server.arg("p1").toFloat(), p1c = (p1 - p1f) / p1f * 100;
  float p2f = server.arg("p2").toFloat(), p2c = (p2 - p2f) / p2f * 100;
  float p3f = server.arg("p3").toFloat(), p3c = (p3 - p3f) / p3f * 100;
  float p4f = server.arg("p4").toFloat(), p4c = (p4 - p4f) / p4f * 100;
  float p5f = server.arg("p5").toFloat(), p5c = (p5 - p5f) / p5f * 100;
  float p6f = server.arg("p6").toFloat(), p6c = (p6 - p6f) / p6f * 100;
  float p7f = server.arg("p7").toFloat(), p7c = (p7 - p7f) / p7f * 100;
  float p8f = server.arg("p8").toFloat(), p8c = (p8 - p8f) / p8f * 100;

  String prc1, prc2, prc3, prc4, prc5, prc6, prc7, prc8;
  if (p1f != NULL and p1f != 0)
  {
    prc1 = "=" + fFTS(p1, 2) + " " + fFTS(p1c, 2) + "%";
  }
  else
  {
    prc1 = "";
  }
  if (p2f != NULL and p2f != 0)
  {
    prc2 = "=" + fFTS(p2, 2) + " " + fFTS(p2c, 2) + "%";
  }
  else
  {
    prc2 = "";
  }
  if (p3f != NULL and p3f != 0)
  {
    prc3 = "=" + fFTS(p3, 2) + " " + fFTS(p3c, 2) + "%";
  }
  else
  {
    prc3 = "";
  }
  if (p4f != NULL and p4f != 0)
  {
    prc4 = "=" + fFTS(p4, 2) + " " + fFTS(p4c, 2) + "%";
  }
  else
  {
    prc4 = "";
  }
  if (p5f != NULL and p5f != 0)
  {
    prc5 = "=" + fFTS(p5, 2) + " " + fFTS(p5c, 2) + "%";
  }
  else
  {
    prc5 = "";
  }
  if (p6f != NULL and p6f != 0)
  {
    prc6 = "=" + fFTS(p6, 2) + " " + fFTS(p6c, 2) + "%";
  }
  else
  {
    prc6 = "";
  }
  if (p7f != NULL and p7f != 0)
  {
    prc7 = "=" + fFTS(p7, 2) + " " + fFTS(p7c, 2) + "%";
  }
  else
  {
    prc7 = "";
  }
  if (p8f != NULL and p8f != 0)
  {
    prc8 = "=" + fFTS(p8, 2) + " " + fFTS(p8c, 2) + "%";
  }
  else
  {
    prc8 = "";
  }

  message += "<form action='st' method='get'>";
  message += "<p>P1 = <input type='text' name='p1' value='" + server.arg("p1") + "'/> " + pump1n + prc1 + "</p>";
  message += "<p>P2 = <input type='text' name='p2' value='" + server.arg("p2") + "'/> " + pump2n + prc2 + "</p>";
  message += "<p>P3 = <input type='text' name='p3' value='" + server.arg("p3") + "'/> " + pump3n + prc3 + "</p>";
  message += "<p>P4 = <input type='text' name='p4' value='" + server.arg("p4") + "'/> " + pump4n + prc4 + "</p>";
  message += "<p>P5 = <input type='text' name='p5' value='" + server.arg("p5") + "'/> " + pump5n + prc5 + "</p>";
  message += "<p>P6 = <input type='text' name='p6' value='" + server.arg("p6") + "'/> " + pump6n + prc6 + "</p>";
  message += "<p>P7 = <input type='text' name='p7' value='" + server.arg("p7") + "'/> " + pump7n + prc7 + "</p>";
  message += "<p>P8 = <input type='text' name='p8' value='" + server.arg("p8") + "'/> " + pump8n + prc8 + "</p>";

  if (wstatus == "Ready")
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






void scales()
{
  String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
  message += "<meta http-equiv='refresh' content='5'>";
  message += "<h3>Current weight = " + fFTS(fscl, 2) + "</h3>";
  message += "RAW = " + fFTS(unitsToRaw(fscl), 0);
  message += "<p><input type='button' class='button' onclick=\"window.location.href = 'tare';\" value='Set to ZERO'/>  ";
  message += "<input type='button' class='button' onclick=\"window.location.href = '/';\" value='Home'/>";
  message += "</p>";

  server.send(200, "text/html", message);
}

void tare()
{
  tareScalesWithCheck(255);
  String message = "<script language='JavaScript' type='text/javascript'>setTimeout('window.history.go(-1)',0);</script>";
  message += "<input type='button' class='button' onclick='history.back();' value='back'/>";

  server.send(200, "text/html", message);
}

void calibrate()
{
  float raw = unitsToRaw(readScalesWithCheck(255));
  String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
  message += "Calibrate (calculate scale_calibration value)";
  message += "<h1>Current RAW = " + fFTS(raw, 0) + "</h1>";
  scale.set_scale(scale_calibration_A);
  message += "<br><h2>Current Value for point A = " + fFTS(rawToUnits(raw), 2) + "g</h2>";
  scale.set_scale(scale_calibration_B);
  message += "<br><h2>Current Value for point B = " + fFTS(rawToUnits(raw), 2) + "g</h2>";
  message += "<br>Current scale_calibration_A = " + fFTS(scale_calibration_A, 4);
  message += "<br>Current scale_calibration_B = " + fFTS(scale_calibration_B, 4);
  message += "<form action='' method='get'>";
  message += "<p>RAW on Zero <input type='text' name='x1' value='" + server.arg("x1") + "'/></p>";
  message += "<p>RAW value with load <input type='text' name='x2' value='" + server.arg("x2") + "'/></p>";
  message += "<p>Value with load (gramm) <input type='text' name='s2' value='" + server.arg("s2") + "'/></p>";
  message += "<p><input type='submit' class='button' value='Submit'/>  ";
  message += "<input type='button' class='button' onclick=\"window.location.href = 'tare';\" value='Set to ZERO'/>  ";
  message += "<button class='button'  onClick='window.location.reload();'>Refresh</button>  ";
  message += "<input class='button' type='button' onclick=\"window.location.href = '/';\" value='Home'/>";
  message += "</p>";

  float x1 = server.arg("x1").toFloat();
  float x2 = server.arg("x2").toFloat();
  float s2 = server.arg("s2").toFloat();
  float k, y, s;

  if (s2 != 0)
  {
    k = -(x1 - x2) / s2;
    message += "<br> scale_calibration = <b>" + fFTS(k, 4) + "</b> copy and paste to your sketch";
  }

  if (x1 > 0 and x2 > 0)
  {
    y = -(s2 * x1) / (x1 - x2);
    message += "<br>Calculate preloaded weight = " + fFTS(y, 2) + "g";
  }

  if (s2 != 0)
  {
    s = raw * (1 / k) - y;
    message += "<br>Calculate weight = " + fFTS(s, 2) + "g";
  }

  server.send(200, "text/html", message);
}





















void setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
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

  Wire.begin();
  // Wire.setClock(400000L);   // 400 kHz I2C clock speed
  lcd.init(); // 
  lcd.backlight();

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  // scale.set_scale(1738.f); //B side
  scale.set_scale(scale_calibration_A); // A side
  lcd.setCursor(0, 0);
  lcd.print("Start FW: ");
  lcd.print(FW_version);
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  scale.power_up();

  wstatus = "Ready";

  server.handleClient();

  delay(3000);
  tareScalesWithCheck(255);
  lcd.clear();
}



void loop()
{
  server.handleClient();
  ArduinoOTA.handle();

#if (KALMAN || !defined(KALMAN))
  float scl = scale.get_units(16);
  fscl = kalmanFilter(scl);
  if (abs(scl - fscl) / fscl > 0.05)
  {
    Xe1 = scl;
  }
#else
  fscl = scale.get_units(128);
#endif
  lcd.setCursor(0, 1);
  lcd.print(fscl, 2);
  lcd.print("         ");

  lcd.setCursor(10, 0);
  lcd.print("Ready  ");
}

