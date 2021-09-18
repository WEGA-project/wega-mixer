///////////////////////////////////////////////////////////////////
// main code - don't change if you don't know what you are doing //
///////////////////////////////////////////////////////////////////
const char FW_version[] PROGMEM = "2.1.5";

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "src/LiquidCrystal_I2C/LiquidCrystal_I2C.h"
#include "src/HX711/src/HX711.h"
#include "src/Adafruit_MCP23017/Adafruit_MCP23017.h"

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

State state;
float goal[PUMPS_NO];
float curvol[PUMPS_NO];
float sumA, sumB;
byte pumpWorking = -1;
unsigned long sTime, eTime;

ESP8266WebServer server(80);
Adafruit_MCP23017 mcp;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F // SDA = D1, SCL = D2
HX711 scale;

#define SSE_MAX_CHANNELS 8
WiFiClient subscription[SSE_MAX_CHANNELS];
unsigned long lastSentTime = 0;

void setState(State s);

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
  delay (3000);
  tareScalesWithCheck(255);  
  
  lcd.clear();
  setState(STATE_READY);
}

void loop() {
  readScales(16);
  printStatus(stateStr[state]); 
  printProgressValueOnly(rawToUnits(displayFilter.getEstimation()));
  server.handleClient();
  ArduinoOTA.handle();
  MDNS.update();
  if (lastSentTime + 1000 < millis()) sendScalesValue();
}

void setState(State s){
  state = s;
  sendState();
}

void handleSubscribe() {
  for (int i = 0; i < SSE_MAX_CHANNELS; i++) {
    if (!subscription[i] || subscription[i].status() == CLOSED) {
      server.client().setNoDelay(true);
      server.client().setSync(true);
      subscription[i] = server.client();
      
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.sendContent_P(PSTR("HTTP/1.1 200 OK\n"
        "Content-Type: text/event-stream;\n" 
        "Connection: keep-alive\n" 
        "Cache-Control: no-cache\n" 
        "Access-Control-Allow-Origin: *\n\n"));

      sendReportUpdate();
      sendState();
      return;
    }
  }
  return busyPage();
}

void sendScalesValue() {
  sendEvent(F("scales"), 256, [] (String& message) {
    appendJson(message, F("value"),    rawToUnits(displayFilter.getEstimation()), false, false);  
    appendJson(message, F("rawValue"), displayFilter.getEstimation(),             false, false);
    appendJson(message, F("rawZero"),  scale.get_offset(),                        false, true);
  }); 
}

void sendState() {
  sendEvent(F("state"), 256, [] (String& message) {
    appendJson(message, F("state"), stateStr[state],  true, true);  
  }); 
}

void sendReportUpdate() {
  sendEvent(F("report"), 512, [] (String& message) {
    unsigned long ms = sTime == 0 ? 0 : (eTime == 0 ? millis() - sTime : eTime - sTime);  
    appendJson(message,    F("timer"),  ms / 1000,         false, false);
    appendJson(message,    F("sumA"),   sumA,              false, false);
    appendJson(message,    F("sumB"),   sumB,              false, false);
    appendJson(message,    F("pumpWorking"), pumpWorking,  false, false); 
    appendJsonArr(message, F("goal"),   goal,   PUMPS_NO,  false, false);
    appendJsonArr(message, F("result"), curvol, PUMPS_NO,  false, true);
  });  
}

void handleMeta() {
  String message((char*)0);
  message.reserve(255);
  message += '{';
  appendJson(message,    F("version"),     FPSTR(FW_version),         true,  false);
  appendJson(message,    F("scalePointA"), scale_calibration_A,       false, false);
  appendJson(message,    F("scalePointB"), scale_calibration_B,       false, false);  
  appendJsonArr(message, F("names"),       names, PUMPS_NO,           true,  true);  
  message += '}';
  server.send(200, "application/json", message);
}

void handleMeasure() {
  if (state != STATE_READY) return busyPage(); 
  
  setState(STATE_BUSY);    
  float rawValue = readScalesWithCheck(128);
  String message((char*)0);
  message.reserve(255);
  message += '{';
  appendJson(message, F("value"),    rawToUnits(rawValue),  false, false);
  appendJson(message, F("rawValue"), rawValue,              false, false);
  appendJson(message, F("rawZero"),  scale.get_offset(),    false, true);
  message += '}';
  server.send(200, "application/json", message);
  setState(STATE_READY);
} 

void handleTare(){
  if (state != STATE_READY) return busyPage(); 

  setState(STATE_BUSY);
  printStatus(stateStr[state]);
  printProgress(F("Taring"));
  tareScalesWithCheck(255);
  setState(STATE_READY);
  okPage();
}

void handleTest(){
  if (state != STATE_READY) return busyPage(); 

  setState(STATE_BUSY);
  okPage();
  for (byte i = 0; i < PUMPS_NO; i++) {
    printStage(i, F("Start")); pumpStart(i); delay(3000);
    printStage(i, F("Revers")); pumpReverse(i); delay(3000);
    printStage(i, F("Stop"));  pumpStop(i); delay(1000);
  }
  setState(STATE_READY);
}

void handleStart() {
  if (state != STATE_READY) return busyPage(); 
  
  setState(STATE_WORKING);  
  sTime = millis();
  eTime = 0;
  sumA = 0;
  sumB = 0;
  int systemId = server.arg("s").toInt();

  for (byte i = 0; i < PUMPS_NO; i ++) {
    goal[i] = server.arg(String(F("p")) + (i + 1)).toFloat();
    curvol[i] = 0;
  }
 
  okPage();

  float offsetBeforePump = scale.get_offset();
  scale.set_scale(scale_calibration_A);
  float raw1 = readScalesWithCheck(255);
  pumping(0);
  pumping(1);
  pumping(2);
  float raw2 = readScalesWithCheck(255);   
  sumA = (raw2 - raw1) / scale_calibration_A;
  
  scale.set_scale(scale_calibration_B); 
  pumping(3);
  pumping(4);
  pumping(5);
  pumping(6);
  pumping(7); 
  pumpWorking = -1;
  
  float raw3 = readScalesWithCheck(255); 
  sumB = (raw3 - raw2) / scale_calibration_B;

  reportToWega(systemId);

  scale.set_scale(scale_calibration_A);
  scale.set_offset(offsetBeforePump);

  eTime = millis();
  sendReportUpdate();
  setState(STATE_READY);
  lcd.clear();
}

void reportToWega(int systemId) {
  String httpstr((char*)0);
  httpstr.reserve(512);
  httpstr += F(WegaApiUrl);
  httpstr += F("?s=");
  httpstr += systemId; 
  for(byte i = 0; i < PUMPS_NO; i++) {
    httpstr += F("&p");
    httpstr += (i + 1);
    httpstr += '=';
    append(httpstr, curvol[i]);
    httpstr += F("&v");
    httpstr += (i + 1);
    httpstr += '=';
    append(httpstr, goal[i]);
  }
  WiFiClient client;
  HTTPClient http;
  http.begin(client, httpstr);
  http.GET();
  http.end();
}

// Функции помп
void pumpStart(int n) {
  #if (RUSTY_MOTOR_KICK)
  mcp.digitalWrite(pinForward[n], LOW); 
  mcp.digitalWrite(pinReverse[n], HIGH);
  delay(4);
  #endif
  mcp.digitalWrite(pinForward[n], HIGH); 
  mcp.digitalWrite(pinReverse[n], LOW);
}
void pumpStop(int n) {
  mcp.digitalWrite(pinForward[n], LOW); 
  mcp.digitalWrite(pinReverse[n], LOW);
}
void pumpReverse(int n) {
  mcp.digitalWrite(pinForward[n], LOW); 
  mcp.digitalWrite(pinReverse[n], HIGH);
}

// Функции вывода на экран
// |1:Ca 34.45 Drops|
void printStage(byte n, const __FlashStringHelper* stage) {
  lcd.clear(); 
  lcd.printf_P(PSTR("%4s %.2f %S     "), names[n], goal[n], stage); 
}

// |          Ready |
void printStatus(const char* status) {
  lcd.setCursor(0, 0);
  lcd.printf("%15s ", status); 
}

// | Skip           |
void printProgress(const __FlashStringHelper* progress) {
  lcd.setCursor(0, 1);
  lcd.print(progress);
}

// | 3.14           |
void printProgressValueOnly(float value) {
  lcd.setCursor(0, 1);
  lcd.printf_P(PSTR("%5.2f           "), truncNegativeZero(value, -0.01)); 
}

float truncNegativeZero (float x, float treshold) {
  return x < 0 && x > treshold ? 0 : x;
}

// | 3.14 80%       |
void printProgress(float value) {
  lcd.setCursor(0, 1);
  lcd.printf_P(PSTR("%5.2f %.0f%%      "), value, value / goal[pumpWorking] * 100); 
}

// | 3.14 80% 80ms  |
void printProgress(float value, int ms) {
  lcd.setCursor(0, 1);
  lcd.printf_P(PSTR("%5.2f %.0f%% %2dms      "), value, value / goal[pumpWorking] * 100, ms); 
}

// | 3.14 100.02%   |
void printResult(float value) {
  lcd.setCursor(0, 1);
  lcd.printf_P(PSTR("%5.2f %.2f%%      "), value, value / goal[pumpWorking] * 100);
}

// |Preload=12000ms |
void printPreload(int ms) {
  lcd.setCursor(0, 1);
  lcd.printf_P(PSTR("Preload=%dms    "), ms);
}

long pumpToValue(byte n, float capValue, float capMillis, float allowedOscillation, void (*printFunc)(float)) {
  float value = rawToUnits(filter.getEstimation());
  if (value >= capValue) return 0;
  
  unsigned long startMillis = millis();
  unsigned long stopMillis = startMillis + capMillis;
  float maxValue = value;
  pumpStart(n);
  char exitCode;
  for (long i = 0; true; i++) { 
    if (value >= capValue) {
      exitCode = 'V'; // вес достиг заданный
      break;
    } else if (millis() >= stopMillis) {
      exitCode = 'T'; // истекло время
      break;
    } else if (value < maxValue - allowedOscillation) {
      exitCode = 'E'; // аномальные показания весов
      break;
    }
    readScales(1);
    value = rawToUnits(filter.getEstimation());
    maxValue = max(value, maxValue);
    if (i % 16 == 0) {
      printFunc(value);
      yield();
      server.handleClient();
      ping();
    }
  }
  pumpStop(n);
  printFunc(value);
  lcd.setCursor(15, 1);lcd.print(exitCode);
  return millis() - startMillis;
}

// Функция налива
float pumping(int n) {
  pumpWorking = n;
  server.handleClient();
  sendReportUpdate();

  if (goal[n] <= 0) {
    printStage(n, F("Skip"));
    printProgress(F("SKIP"));
    wait(1000, 10);
    return 0;
  }

  printStage(n, F("Tare"));
  tareScalesWithCheck(255);
  server.handleClient();

  printStage(n, F("Load"));
  long preload = 0;
  if (goal[n] < 0.5) { // статический прелоад
    preload = staticPreload[n];

    printProgress(F("Reverse"));
    pumpReverse(n); wait(preload, 10); pumpStop(n);
    printPreload(preload);
    pumpStart(n); wait(preload, 10); pumpStop(n);

    curvol[n] = rawToUnits(readScalesWithCheck(128));
    server.handleClient();
    sendReportUpdate();
  } else { // прелоад до первой капли
    while (curvol[n] < 0.02) {
      preload += pumpToValue(n, 0.03, staticPreload[n] * 2, 0.1, printProgressValueOnly);
      curvol[n] = rawToUnits(readScalesWithCheck(128));
      server.handleClient();
      sendReportUpdate();
    }
    printPreload(preload);
    wait(2000, 10);
  }
  
  // быстрая фаза до конечного веса минус 0.2 - 0.5 грамм по половине от остатка 
  printStage(n, F("Fast"));
  float performance = 0.0007;                                 // производительность грамм/мс при 12v и трубке 2x4, в процессе уточняется
  float dropTreshold = goal[n] - curvol[n] > 1.0 ? 0.2 : 0.5; // определяет сколько оставить на капельный налив. Чем больше итераций тем точнее и можно лить почти до конца
  float valueToPump = goal[n] - curvol[n] - dropTreshold;
  float allowedOscillation = valueToPump < 1.0 ? 1.5 : 3.0;  // допустимое раскачивание, чтобы остановиться при помехах/раскачивании, важно на малых объемах  
  if (valueToPump > 0.3) { // если быстро качать не много то не начинать даже
    while ((valueToPump = goal[n] - curvol[n] - dropTreshold) > 0) {
      if (valueToPump > 0.2) valueToPump = valueToPump / 2;  // качать по половине от остатка
      long timeToPump = valueToPump / performance;           // ограничение по времени
      long workedTime = pumpToValue(n, curvol[n] + valueToPump, timeToPump, allowedOscillation, printProgress);
      float prevValue = curvol[n];
      curvol[n] = rawToUnits(readScalesWithCheck(128));
      if (workedTime > 200 && curvol[n] - prevValue > 0.15) performance = max(performance, (curvol[n] - prevValue) / workedTime);
      server.handleClient();
      sendReportUpdate();
    }
  }
  
  // капельный налив
  printStage(n, F("Drop"));
  int sk = 25;
  while (curvol[n] < goal[n] - 0.01) {
    printProgress(curvol[n], sk);
    
    pumpStart(n); delay(sk); pumpStop(n);
      
    float prevValue = curvol[n];
    curvol[n] = rawToUnits(readScalesWithCheck(128));
    if (curvol[n] - prevValue < 0.01) {sk = min(80, sk + 2);}
    if (curvol[n] - prevValue > 0.01) {sk = max(2, sk - 2);}
    if (curvol[n] - prevValue > 0.1 ) {sk = 0;}
    
    server.handleClient();
    sendReportUpdate();
  }

  // реверс, высушить трубки
  printStage(n, F("Dry"));
  printResult(curvol[n]);
  pumpReverse(n);
  wait(max(preload, staticPreload[n]) * 1.5, 10); 
  pumpStop(n);
}

void wait(unsigned long ms, int step) {
  unsigned long endPreloadTime = millis() + ms; 
  while (millis() < endPreloadTime) {
    server.handleClient();
    ping();
    delay(step);
  }
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
    server.handleClient();
    ping();
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

// функции для генерации json
template<typename T>
void appendJson(String& src, const __FlashStringHelper* name, const T& value, const bool quote, const bool last) {
  src += '"'; src += name; src += F("\":");
  if (quote) src += '"';
  append(src, value);
  if (quote) src += '"';
  if (!last) src += ',';  
}

template<typename T>
void appendJsonArr(String& src, const __FlashStringHelper* name, const T value[], const int len, const bool quote, const bool last) {
  src += '"'; src += name; src += F("\":");
  src += '[';
  for (int i = 0; i < len; i++) {
    if (quote) src += '"';
    append(src, value[i]);
    if (quote) src += '"';
    if (i < (len - 1)) src += ',';
  }
  src += ']';
  if (!last) src += ',';  
}

template<typename T>
void append(String& ret, const T& value) {
  ret += value;  
}
template <>
void append<float>(String& ret, const float& value) {
  char buff[20];
  dtostrf(value, 5, 3, buff);
  ret += buff;
}

// server sent events
bool checkConnected() {
  bool result = false;
  for (uint8_t i = 0; i < SSE_MAX_CHANNELS; i++) {
    if (!subscription[i] || subscription[i].status() == CLOSED) {
      continue;
    } else if (subscription[i].connected()) {
      result = true;
    } else {
      subscription[i].flush();
      subscription[i].stop();
    }
  }
  return result;
}

void sendEvent(const __FlashStringHelper* name, int bufSize, void (*appendPayload)(String&)) {
  if (!checkConnected()) return;
  lastSentTime = millis();
  String message((char*)0);
  message.reserve(bufSize);
  message += F("event:");
  message += name;
  message += F("\ndata:{");
  appendPayload(message);
  message += F("}\n\n");
  
  for (byte i = 0; i < SSE_MAX_CHANNELS; i++) {
    if (subscription[i] && subscription[i].connected()) subscription[i].print(message); 
  }
}

void ping() {
  if (lastSentTime + 10000 > millis()) return;
  lastSentTime = millis();
  if (!checkConnected()) return;
  
  for (byte i = 0; i < SSE_MAX_CHANNELS; i++) {
    if (subscription[i] && subscription[i].connected()) { 
      subscription[i].print(F(": keepAlive\n\n"));
    }   
  }
}
