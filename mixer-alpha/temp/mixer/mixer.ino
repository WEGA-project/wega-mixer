////////////////////
// User variables //
////////////////////

// WiFi network configuration
const char* ssid = "dd-wrt";
const char* password = "HruBse1447209";

// scale_calibration, side A and B
float scale_calibration_A = 1744.2507;
float scale_calibration_B = 1734.3755;


// Setting WEGA-API
#define WegaApiUrl "http://192.168.237.107/remote/mixerdb.php"

// Pump #1
#define pump1  B3        // Port 1
#define pump1r A3        // Port 2
#define pump1n "1:Ca"    // Display name
#define pump1p 6790      // Preload (ms)

// Pump #2
#define pump2  A2
#define pump2r B2
#define pump2n "2:NK"
#define pump2p 5000

// Pump #3
#define pump3  A1
#define pump3r B1
#define pump3n "3:NH"
#define pump3p 7000

// Pump #4
#define pump4  A0
#define pump4r B0
#define pump4n "4:Mg"
#define pump4p 5000

// Pump #5
#define pump5  A7
#define pump5r B7
#define pump5n "5:KP"
#define pump5p 5000

// Pump #6
#define pump6  A6
#define pump6r B6
#define pump6n "6:KS"
#define pump6p 5000

// Pump #7
#define pump7  A5
#define pump7r B5
#define pump7n "7:MK"
#define pump7p 5000

// Pump #8
#define pump8  A4
#define pump8r B4
#define pump8n "8:B"
#define pump8p 5000
