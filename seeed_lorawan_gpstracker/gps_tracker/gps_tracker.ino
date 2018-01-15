#include "TinyGPS++.h"
#include <LoRaWan.h>

#define DEFAULT_RESPONSE_TIMEOUT 5

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
  // For debugging
  // while(!SerialUSB);

  Serial.begin(9600);

  lora.init();

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

  lora.setPower(14);

  while (!lora.setOTAAJoin(JOIN))
    ;

  SerialUSB.print("Testing TinyGPS++");
}

void loop() {

  bool result = false;

  if (gps.altitude.isUpdated())
    SerialUSB.println(gps.altitude.meters());

  if (gps.location.isUpdated()) {
    SerialUSB.print("LAT=");
    SerialUSB.print(gps.location.lat(), 6);
    SerialUSB.print("LNG=");
    SerialUSB.println(gps.location.lng(), 6);

    latlong.f[0] = gps.location.lat();
    latlong.f[1] = gps.location.lng();

    SerialUSB.println("LatLong: ");
    for (int i = 0; i < 8; i++) {
      SerialUSB.print(latlong.bytes[i], HEX);
    }
    SerialUSB.println("");
    result = lora.transferPacket(latlong.bytes, 8, DEFAULT_RESPONSE_TIMEOUT);

  } else {
    SerialUSB.println("No GPS Location");
    result = lora.transferPacket("No GPS Location", 10);
  }

  if (result) {
    short length;
    short rssi;

    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi);

    if (length) {
      SerialUSB.print("Length is: ");
      SerialUSB.println(length);
      SerialUSB.print("RSSI is: ");
      SerialUSB.println(rssi);
      SerialUSB.print("Data is: ");
      for (unsigned char i = 0; i < length; i++) {
        SerialUSB.print("0x");
        SerialUSB.print(buffer[i], HEX);
        SerialUSB.print(" ");
      }
      SerialUSB.println();
    }
  }

  smartdelay(1000);
}

static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (Serial.available())
      gps.encode(Serial.read());
  } while (millis() - start < ms);
}
