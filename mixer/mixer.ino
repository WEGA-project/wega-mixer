////////////////////
// User variables //
////////////////////

//////////////////////////////////////////////////////////////
// In order to ignore changes in this file you need to run  //
// git update-index --assume-unchanged mixer/mixer.ino      //
// -------------------------------------------------------- //
// To start tracking changes in this file again run         //
// git update-index --no-assume-unchanged mixer/mixer.ino   //
//////////////////////////////////////////////////////////////

// WiFi network configuration
const char* ssid = "YOUR_WIFI_NETWORK_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// scale_calibration, side A and B
float scale_calibration_A = 2030;
float scale_calibration_B = 2030;

//

#define board_ESP32
// Kalman filtering in scales mode
#define KALMAN true

// Setting WEGA-API
#define WegaApiUrl "http://192.168.2.66/remote/mixerdb.php"

// Pump #1
#define pump1  A0        // Port 1
#define pump1r B0        // Port 2
#define pump1n "1:Ca"    // Display name
#define pump1p 4000      // Preload (ms)

// Pump #2
#define pump2  A1
#define pump2r B1
#define pump2n "2:NK"
#define pump2p 4000

// Pump #3
#define pump3  A2
#define pump3r B2
#define pump3n "3:NH"
#define pump3p 3500

// Pump #4
#define pump4  A3
#define pump4r B3
#define pump4n "4:Mg"
#define pump4p 4000

// Pump #5
#define pump5  A4
#define pump5r B4
#define pump5n "5:KP"
#define pump5p 4000

// Pump #6
#define pump6  A5
#define pump6r B5
#define pump6n "6:KS"
#define pump6p 4000

// Pump #7
#define pump7  A6
#define pump7r B6
#define pump7n "7:MK"
#define pump7p 4000

// Pump #8
#define pump8  A7
#define pump8r B7
#define pump8n "8:B"
#define pump8p 4000
