#include "TinyGPS++.h"
#include <LoRaWanPlus.h>

#define DEFAULT_RESPONSE_TIMEOUT 15

// 0-26 pins
#define BLUELED 14 
#define GREENLED 16 

TinyGPSPlus gps;

char buffer[256];

typedef union {
  float f[2]; // Assigning fVal.f will also populate fVal.bytes;
  unsigned char
      bytes[8]; // Both fVal.f and fVal.bytes share the same 4 bytes of memory.
} floatArr2Val;
floatArr2Val latlong;

static void smartdelay(unsigned long ms);

void setup() {
    SerialUSB.begin(115200);

      lora.init();




  // enable power to grove components
  digitalWrite(38, HIGH);

               pinMode(GREENLED, OUTPUT);
      pinMode(BLUELED, OUTPUT);






  
 
  Serial.begin(9600);


  memset(buffer, 0, 256);
  lora.getVersion(buffer, 256, 1);
  SerialUSB.print(buffer);

  memset(buffer, 0, 256);
  lora.getId(buffer, 256, 1);
  SerialUSB.print(buffer);

  // void setId(char *DevAddr, char *DevEUI, char *AppEUI);
  lora.setId(NULL, "00F33FDD11542C5B", "70B3D57ED00094C3");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey(NULL, NULL, "3AFE4E0D4A5EE9534613B7E147C5DF20");

  lora.setDeciveMode(LWOTAA);
  lora.setDataRate(DR0, EU868);

  lora.setChannel(0, 868.1);
  lora.setChannel(1, 868.3);
  lora.setChannel(2, 868.5);

  lora.setReceiceWindowFirst(0, 868.1);
  lora.setReceiceWindowSecond(869.5, DR3);

  lora.setDutyCycle(false);
  lora.setJoinDutyCycle(false);

  // 0 > 10dbm // 14 RFU
  lora.setPower(0);

  while (!lora.setOTAAJoin(JOIN));
}

void loop() {

  bool result = false;

  if (gps.altitude.isUpdated())
    SerialUSB.println(gps.altitude.meters());

  if (gps.location.isUpdated()) {
    SerialUSB.print("+++LAT=");
    SerialUSB.print(gps.location.lat(), 6);
    SerialUSB.print("+++LNG=");
    SerialUSB.println(gps.location.lng(), 6);

    latlong.f[0] = gps.location.lat();
    latlong.f[1] = gps.location.lng();

        digitalWrite(BLUELED, HIGH);

    result = lora.transferPacket(latlong.bytes, 8, DEFAULT_RESPONSE_TIMEOUT);

  } else {
        digitalWrite(BLUELED, LOW);
    SerialUSB.println("+++No GPS Location");
latlong.f[0] = 0;
    latlong.f[1] = 0;

    result = lora.transferPacketWithConfirmed(latlong.bytes, 8, DEFAULT_RESPONSE_TIMEOUT);
  }

  if (!result) {
      // failed message we are not joined anymore
      SerialUSB.println("+++RE-Joining Network");
        digitalWrite(GREENLED, LOW);
        setup();
    } else {
        digitalWrite(GREENLED, HIGH);
    }

  smartdelay(6000);
}

static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (Serial.available())
      gps.encode(Serial.read());
  } while (millis() - start < ms);
}

