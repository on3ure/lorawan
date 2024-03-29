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

char buffer[128] = {0};

// The TinyGPS++ object
TinyGPSPlus gps;
typedef union {
  float f[3]; 
  unsigned char
      bytes[12]; 
} floatArr2Val;

floatArr2Val latlong;

float latitude;
float longitude;

void setupLoRaOTAA() {
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
  lora.setId(NULL, "006C25E52699D74C", "70B3D57ED0009548");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey(NULL, NULL, "260A16EACA3D43353BA050FBA8C9C582");

  lora.setAdaptiveDataRate(true);
  lora.setPower(MAX_EIRP_NDX_EU);
  lora.setReceiveWindowSecond(FREQ_RX_WNDW_SCND_EU, DOWNLINK_DATA_RATE_EU);
 
   if (lora.setDeviceMode(LWOTAA) == false)               // Over The Air Activation
      SerialUSB.print("++Set Mode to OTAA failed.\n");
  else
      SerialUSB.print("++OTAA mode set.\n");
 
  while(true) {
      if (lora.setOTAAJoin(JOIN))
        break;
    }
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
  setupLoRaOTAA();

  // we are ready turn on the light
  digitalWrite(GREENLED, HIGH);
}

void loop() {
    while (Serial.available() > 0) {
      char currChar = Serial.read();
      gps.encode(currChar);
    }
    if (gps.location.isUpdated()) {
      SerialUSB.println(gps.date.value()); // Raw date in DDMMYY format (u32)
      SerialUSB.println(gps.time.value()); // Raw time in HHMMSSCC format (u32)
      SerialUSB.println(gps.speed.kmph()); // Speed in kilometers per hour (double)
      SerialUSB.println(gps.course.value()); // Raw course in 100ths of a degree (i32)
      SerialUSB.println(gps.course.deg()); // Course in degrees (double)
      SerialUSB.println(gps.satellites.value()); // Number of satellites in use (u32)

      latitude = gps.location.lat();
      longitude = gps.location.lng();
      if ((latitude && longitude) && latitude != latlong.f[0] &&
          longitude != latlong.f[1]) {
        latlong.f[0] = gps.location.lat();
        latlong.f[1] = gps.location.lng();
        latlong.f[2] = gps.altitude.feet();
  
        SerialUSB.print("++sendPacket LatLong: ");
        for (int i = 0; i < 8; i++) {
          SerialUSB.print(latlong.bytes[i], HEX);
        }
        SerialUSB.println();
        
        SerialUSB.print("++Location: ");
        SerialUSB.print(latlong.f[0], 6);
        SerialUSB.print(",");
        SerialUSB.print(latlong.f[1], 6);
        SerialUSB.print(" Latitude: ");
        SerialUSB.print(latlong.f[2], 6);
        SerialUSB.println();

        bool result =
            lora.transferPacket(latlong.bytes, 12, DEFAULT_RESPONSE_TIMEOUT);
        digitalWrite(BLUELED, HIGH);
      } else {
        digitalWrite(BLUELED, LOW);
      }
    }
}
