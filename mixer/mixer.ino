////////////////////
// User variables //
////////////////////

// scale_calibration
float scale_calibration = 2030;

// WiFi network configuration
const char* ssid = "YOUR_WIFI_NETWORK_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Here is the naming convention:
// A0=0 .. A7=7
// B0=8 .. B7=15
// That means if you have A0 == 0 B0 == 8 (this is how it's on the board B0/A0 == 8/0)
// one more example B3/A3 == 11/3

// Pump #1
int a0 = 0;
int b0 = 8;

// Pump #2
int a1 = 1;
int b1 = 9;

// Pump #3
int a2 = 2;
int b2 = 10;

// Pump #4
int a3 = 3;
int b3 = 11;

// Pump #5
int a4 = 4;
int b4 = 12;

// Pump #6
int a5 = 5;
int b5 = 13;

// Pump #7
int a6 = 6;
int b6 = 14;

// Pump #8
int a7 = 7;
int b7 = 14;

