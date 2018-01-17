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

void setHybridForTTN(const float *channels) {
  for (int i = 0; i < 8; i++) {
    if (channels[i] != 0) {
      lora.setChannel(i, channels[i], UPLINK_DATA_RATE_MIN,
                      UPLINK_DATA_RATE_MAX_EU);
    }
  }
}

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
  lora.setId(NULL, "00C14D75CFD10491", "70B3D57ED0009548");
  //lora.setId(NULL, NULL, "70B3D57ED00094C3");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey("89BFC68717D6CEB03F80FC54CEB43A26", "89BFC68717D6CEB03F80FC54CEB43A26", "89BFC68717D6CEB03F80FC54CEB43A26");


  if (lora.setDeviceMode(LWOTAA) == false)               // Over The Air Activation
      SerialUSB.print("++Set Mode to OTAA failed.\n");
  else
      SerialUSB.print("++OTAA mode set.\n");

  lora.setAdaptiveDataRate(true);
  lora.setPower(MAX_EIRP_NDX_EU);
  lora.setReceiveWindowSecond(FREQ_RX_WNDW_SCND_EU, DOWNLINK_DATA_RATE_EU);
 
  
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
      digitalWrite(BLUELED, HIGH);
    } else {
      digitalWrite(BLUELED, LOW);
    }
  }
}
