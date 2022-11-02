#include <Arduino.h>

///////////////////////////////////////////////////////////////////
// main code - don't change if you don't know what you are doing //
///////////////////////////////////////////////////////////////////
const char FW_version[] PROGMEM = "2.3.0";

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <Adafruit_MCP23017.h>
#include <PubSubClient.h>

#include <EEPROM.h>
#include <config.h>

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
Kalman filter        = Kalman(1000, 80, 0.4);  // резкий

#define PUMPS_NO 8
const char* names[PUMPS_NO]        = {pump1n, pump2n, pump3n, pump4n, pump5n, pump6n, pump7n, pump8n};
const byte pinForward[PUMPS_NO]    = {pump1,  pump2,  pump3,  pump4,  pump5,  pump6,  pump7,  pump8};
const byte pinReverse[PUMPS_NO]    = {pump1r, pump2r, pump3r, pump4r, pump5r, pump6r, pump7r, pump8r};
const long staticPreload[PUMPS_NO] = {pump1p, pump2p, pump3p, pump4p, pump5p, pump6p, pump7p, pump8p};
const char* stateStr[]             = {"Ready", "Working", "Busy"};
enum State {STATE_READY, STATE_WORKING, STATE_BUSY};


float goal[PUMPS_NO];
float curvol[PUMPS_NO];
float sumA, sumB;

byte pumpWorking = -1;
unsigned long sTime, eTime;


Adafruit_MCP23017 mcp;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F // SDA = D1, SCL = D2
HX711 scale;
ESP8266WebServer server(80);
State state;
void setState(State s);

#include <func.h>


void setup() {
  Wire.begin(D1, D2);
  lcd.init(); 
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(F("ver: "));
  lcd.print(FPSTR(FW_version));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  lcd.setCursor(0, 1); 
  lcd.print(WiFi.localIP()); 

  MDNS.begin("mixer");
  MDNS.addService("http", "tcp", 80);
  server.on("/rest/events",  handleSubscribe);
  server.on("/rest/meta",    handleMeta);
  server.on("/rest/start",   handleStart);
  server.on("/rest/tare",    handleTare);
  server.on("/rest/measure", handleMeasure);  
  server.on("/rest/test",    handleTest);
  server.on("/rest/test-api",    handleTestApi);
  
  server.on("/",             mainPage);
  server.on("/calibration",  calibrationPage);
  server.on("/style.css",    cssPage);
  server.begin();
  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();


  mcp.begin();
  for (byte i = 0; i < PUMPS_NO; i++) {
    mcp.pinMode(pinForward[i], OUTPUT); 
    mcp.pinMode(pinReverse[i], OUTPUT);
  }

  scale.begin(D5, D6); // DOUT = D5 SCK = D6;
  scale.set_scale(scale_calibration_A);
  scale.power_up();
  tareScalesWithCheck(scale_read_times);  
  lcd.clear();
  setState(STATE_READY);
  readScales(scale_read_times);
  uint16_t buf_size= 512;
  mqqt_client.setBufferSize(buf_size);
  mqqt_client.setServer(calc_url, calc_mqtt_port);
  mqqt_client.connect(calc_token, calc_mqtt_user, calc_mqtt_password); 
   
 
 
  
  EEPROM.get(0, curvol);  
  EEPROM.get(1, goal);  
  EEPROM.get(2, staticPreload);  


}

void loop() {
  readScales(scale_read_times);
  printStatus(stateStr[state]); 
  printProgressValueOnly(rawToUnits(displayFilter.getEstimation()));
  server.handleClient();
  ArduinoOTA.handle();
  MDNS.update();
  if (lastSentTime + 5000 < millis()) {
    lastSentTime = millis();
    sendScalesValue();  
    }
  if (!mqqt_client.loop()){ mqqt_client.connect(calc_token, calc_mqtt_user, calc_mqtt_password); }
  delay(100);
}
