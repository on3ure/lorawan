#include <LoRaWanURE.h>
#include <TimerTCC0.h>
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
int sec = 0;

TinyGPSPlus gps;
typedef union {
  float f[3]; 
  unsigned char
      bytes[12]; 
} floatArr2Val;

floatArr2Val latlong;

float latitude;
float longitude;

void displayInfo() {
  SerialUSB.print("++Location: ");
  SerialUSB.print(latlong.f[0], 6);
  SerialUSB.print(",");
  SerialUSB.print(latlong.f[1], 6);
  SerialUSB.print(" Latitude: ");
  SerialUSB.print(latlong.f[2], 6);
  SerialUSB.println();
}

void timerIsr(void) // interrupt routine
{
  displayInfo();
}

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
  TimerTcc0.initialize(15000000);
  TimerTcc0.attachInterrupt(timerIsr);
}

void loop() {
  if (sec <= 2) {
    while (Serial.available() > 0) {
      char currChar = Serial.read();
      gps.encode(currChar);
    }
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    if ((latitude && longitude) && latitude != latlong.f[0] &&
        longitude != latlong.f[1]) {
      digitalWrite(BLUELED, HIGH);
      latlong.f[0] = latitude;
      latlong.f[1] = longitude;
      latlong.f[2] = gps.altitude.meters();

      SerialUSB.print("++sendPacket LatLong: ");
      for (int i = 0; i < 8; i++) {
        SerialUSB.print(latlong.bytes[i], HEX);
      }
      SerialUSB.println();
      bool result =
          lora.transferPacket(latlong.bytes, 12, DEFAULT_RESPONSE_TIMEOUT);
    } else {
      digitalWrite(BLUELED, LOW);
    }
  }
}
