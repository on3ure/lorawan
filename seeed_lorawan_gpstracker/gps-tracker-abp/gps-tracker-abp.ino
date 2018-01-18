#include <LoRaWanURE.h>
#include <TinyGPS++.h>

// 2nd receive freq
#define FREQ_RX_WNDW_SCND_EU 869.525

// Transmit Power
#define MAX_EIRP_NDX_EU 14

// Downlink Data RATE
#define DOWNLINK_DATA_RATE_EU DR3

// Default response timeout
#define DEFAULT_RESPONSE_TIMEOUT 5

// Blue LED
#define BLUELED 14

// Green LED
#define GREENLED 16

// last update
unsigned long previousMillis = 0;

// interval between LoRaWan updates  
const long interval = 15000; //15 secs
         
char buffer[128] = {0};

// minimum speed to send updates
const double minSpeed = 10; // 10 kmh at least to push an update

// check speed and course
double currentSpeed = 0;
char* currentDirection = {0};

// booleans
boolean trigger = 0;
boolean firstUpdate = 0;

TinyGPSPlus gps;
typedef union {
  float f[3]; 
  unsigned char
      bytes[12]; 
} floatArr2Val;

floatArr2Val latlong;

float latitude;
float longitude;

void setupLoRaABP() {
  lora.init();
  lora.setDeviceReset();
  lora.setDeviceDefault();

  memset(buffer, 0, 256);
  lora.getVersion(buffer, 256, 1);
  SerialUSB.print(buffer);

  memset(buffer, 0, 256);
  lora.getId(buffer, 256, 1);
  SerialUSB.print(buffer);

  // void setId(char *DevAddr, char *DevEUI, char *AppEUI);
  lora.setId("26011CC6", "00050197D45823E9", "70B3D57ED0009548");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey("22137B20EC10E8BEC32C11EBFB1681F0",
              "7D412196E418FA04CB8B1C080DCFC5C2",
              "NULL");

  lora.setDeviceMode(LWABP);
  lora.setAdaptiveDataRate(true);
  lora.setPower(MAX_EIRP_NDX_EU);
  lora.setReceiveWindowSecond(FREQ_RX_WNDW_SCND_EU, DOWNLINK_DATA_RATE_EU);
}

void setup() {
  Serial.begin(9600);
  SerialUSB.begin(115200);

  memset(buffer, 0, 256);

  // enable power to grove components
  digitalWrite(38, HIGH);

  // set pin`s to output modules
  pinMode(GREENLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);

  // setup LoRa
  setupLoRaABP();

  // we are ready turn on the light
  digitalWrite(GREENLED, HIGH);
}

void loop() {
    while (Serial.available() > 0) {
        char currChar = Serial.read();
        gps.encode(currChar);

      if (gps.location.isUpdated()) {
        //SerialUSB.println("++Got location update from GPS");
        latitude = gps.location.lat();
        longitude = gps.location.lng();

        // stuff to add to display
        //SerialUSB.println(gps.date.value()); // Raw date in DDMMYY format (u32)
        //SerialUSB.println(gps.time.value()); // Raw time in HHMMSSCC format (u32)
        //SerialUSB.println(gps.speed.kmph()); // Speed in kilometers per hour (double)
        //SerialUSB.println(gps.course.deg()); // Course in degrees (double) 0-360
        //SerialUSB.println(gps.satellites.value()); // Number of satellites in use (u32)

        if ( strdup(gps.cardinal(gps.course.deg())) != currentDirection && gps.speed.kmph() >= minSpeed ){
           currentDirection = strdup(gps.cardinal(gps.course.deg()));
           SerialUSB.print("++Course Changed to: ");
           SerialUSB.println(currentDirection);
           trigger = 1;
        }
          
      if (firstUpdate != 1 || trigger != 0) {
        if (firstUpdate != 1) 
          SerialUSB.println("++Send First Lock Location");
      
        trigger = 0;
        firstUpdate = 1;
        
        digitalWrite(BLUELED, HIGH);
        latlong.f[0] = latitude;
        latlong.f[1] = longitude;
        latlong.f[2] = gps.altitude.feet();

        
        unsigned long currentMillis = millis();  
        if (currentMillis - previousMillis >= interval) {
          // save the last time we send to the LoRaWan Network
          previousMillis = currentMillis;
          digitalWrite(BLUELED, HIGH);
        
          SerialUSB.print("++Location: ");
          SerialUSB.print(latlong.f[0], 6);
          SerialUSB.print(",");
          SerialUSB.print(latlong.f[1], 6);
          SerialUSB.print(" Altitude (in feet): ");
          SerialUSB.print(latlong.f[2], 6);
          SerialUSB.println();
          
          SerialUSB.print("++sendPacket LatLong: ");
          for (int i = 0; i < 8; i++) {
            SerialUSB.print(latlong.bytes[i], HEX);
          }
          SerialUSB.println();
          bool result =
              lora.transferPacket(latlong.bytes, 12, DEFAULT_RESPONSE_TIMEOUT);
          digitalWrite(BLUELED, LOW);
        } 
          }
    } 
    }
    }
