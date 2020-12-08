
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
const char* ssid = "YOUR_WIFI_NETWORK_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
ESP8266WebServer server(80);


void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500);}
  MDNS.begin("TEST-8266");
  MDNS.addService("http", "tcp", 80);
  server.on("/", handleRoot);
  server.begin();
  ArduinoOTA.onStart([]() { String type;if (ArduinoOTA.getCommand() == U_FLASH){type = "sketch";} else {type = "filesystem";}  });
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();
}


void handleRoot() {
  server.send(200, "text/html", "hi all");
   }


void loop() {
  server.handleClient();
  ArduinoOTA.handle();

}
