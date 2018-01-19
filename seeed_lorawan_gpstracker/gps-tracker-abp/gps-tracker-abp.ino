// TODO: put LORA and GPS to sleep mode ... out of sleep mode when movement
// TODO: Safe frame number when lipo batery is low and shutdown ... read frame number from epron
// TODO: Battery management
// TODO: Add OLED Display option (date time / lat long alt / speed + direction)
// TODO: Add LED Solution (2 or 3 leds)

#include <LoRaWanURE.h>
#include <EnergySaving.h>
#include "MC20_Common.h"
#include "MC20_Arduino_Interface.h"
#include "MC20_GNSS.h"
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



const int pin_battery_voltage = A4;

// last update
unsigned long previousMillis = 0;

// interval between LoRaWan updates
const long interval = 40000; // 40 secs

char buffer[128] = {0};

const double drivingSpeed = 10; 
const double standstillSpeed = 1; 

// check speed and course
double currentSpeed = 0;
char *currentDirection = {0};

// booleans
boolean trigger = 0;
boolean firstUpdate = 0;
boolean standStill = 0;

// options
const boolean hasBattery = 0;

TinyGPSPlus gps;
typedef union {
  float f[3];
  unsigned char bytes[12];
} floatArr2Val;

floatArr2Val latlong;

float latitude;
float longitude;

EnergySaving nrgSave;

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

  memset(buffer, 0, 256);
  lora.getCounters(buffer, 256, 1);
  SerialUSB.print(buffer);

  // void setId(char *DevAddr, char *DevEUI, char *AppEUI);
  lora.setId("26011CC6", "00050197D45823E9", "70B3D57ED0009548");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey("22137B20EC10E8BEC32C11EBFB1681F0",
              "7D412196E418FA04CB8B1C080DCFC5C2", "NULL");

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

  
}

void loop() {
  while (Serial.available() > 0) {

    if (hasBattery) {
      int a = analogRead(pin_battery_voltage);
      float v = a/1023.0*3.3*2.0;        // there's an 10M and 10M resistor divider
      SerialUSB.print("The voltage of battery is ");
      SerialUSB.print(v, 2);
      SerialUSB.println(" V");
      // check batery level when low ... go to deep sleep forever
      if (1) {
        nrgSave.begin(WAKE_EXT_INTERRUPT, 3, dummy);  // write LoRaFrame before shutdown
        SerialUSB.println("MCU Stanby... Sleepy sleep in 30s from now ...");
        delay(30000); // Wait for LoRaFrame to be written to EEPROM
        Serial1.end(); // Disable Uart
        pinMode(0, OUTPUT);
        pinMode(1, OUTPUT);
        digitalWrite(0, LOW); // Rx
        digitalWrite(1, LOW); // Tx
        digitalWrite(9, LOW); // DTR  90uA
        /*******************************************/
        nrgSave.standby();  //now mcu goes in standby mode
      }
    }
    
    char currChar = Serial.read();
    gps.encode(currChar);

    if (gps.location.isUpdated()) {
      digitalWrite(GREENLED, HIGH);
      
      // SerialUSB.println("++Got location update from GPS");
      latitude = gps.location.lat();
      longitude = gps.location.lng();

      // stuff to add to display
      // SerialUSB.println(gps.date.value()); // Raw date in DDMMYY format (u32)
      // SerialUSB.println(gps.time.value()); // Raw time in HHMMSSCC format
      // (u32)  SerialUSB.println(gps.speed.kmph()); // Speed in kilometers per
      // hour (double)  SerialUSB.println(gps.course.deg()); // Course in degrees
      // (double) 0-360  SerialUSB.println(gps.satellites.value()); // Number of
      // satellites in use (u32)

      if (strdup(gps.cardinal(gps.course.deg())) != currentDirection &&
          gps.speed.kmph() >= drivingSpeed) {
        currentDirection = strdup(gps.cardinal(gps.course.deg()));
        SerialUSB.print("++Course Changed to: ");
        SerialUSB.println(currentDirection);
        trigger = 1;
        standStill = 0;
      }

      if (gps.speed.kmph() <= standstillSpeed && standStill == 0) {
        trigger = 1;
        standStill = 1;
      }

      if (firstUpdate != 1 || trigger != 0) {
        if (firstUpdate != 1)
          SerialUSB.println("++Send First Lock Location");
        
        if (standStill == 1) {
          SerialUSB.println("++We are not driving");
        }

        trigger = 0;
        firstUpdate = 1;
        

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
    } else {
      SerialUSB.println("++Waiting for GPS");
      digitalWrite(GREENLED, HIGH);
      delay(500);
      digitalWrite(GREENLED, LOW);
      delay(500);
      digitalWrite(BLUELED, HIGH);
      delay(500);
      digitalWrite(BLUELED, LOW);
      delay(500);
    }
  }
}

void dummy(void)  //interrupt routine (isn't necessary to execute any tasks in this routine
{
  
}
