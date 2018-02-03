// TODO: put LORA and GPS to sleep mode ... out of sleep mode when movement
// TODO: Safe frame number when lipo batery is low and shutdown ... read frame
// number from epron
// TODO: Battery management
// TODO: Add OLED Display option (date time / lat long alt / speed + direction)
// TODO: Add LED Solution (2 or 3 leds)
// https://github.com/cmaglie/FlashStorage

#include <EnergySaving.h>
#include <LoRaWanURE.h>
//#include "MC20_Common.h"
//#include "MC20_Arduino_Interface.h"
//#include "MC20_GNSS.h"
#include <FlashStorage.h>
#include <TinyGPS++.h>

FlashStorage(my_flash_store, int);

// 2nd receive freq
#define FREQ_RX_WNDW_SCND_EU 869.525

// Transmit Power
#define MAX_EIRP_NDX_EU 14

// Downlink Data RATE
#define DOWNLINK_DATA_RATE_EU DR3

// Default response timeout
#define DEFAULT_RESPONSE_TIMEOUT 5

// Blue LED
#define BLUELED A1

// Green LED
#define GREENLED A0

// last update
unsigned long updateTimeLORAsend = 0;
unsigned long updateTimeLORAsendStandingStill = 0;
unsigned long updateTimeGPSlock = 0;
unsigned long updateTimeBATTERYlock = 0;

// interval between LoRaWan updates
const long LORAinterval = 60000;  // each minute
const long LORAintervalStandingStill = 600000;  // each 10 minutes

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
const boolean hasBattery = 1;

// batery stuff
const int pin_battery_status = A5;
const int pin_battery_voltage = A4;

float bat_low = 0;
float bat_high = 0;

TinyGPSPlus gps;
typedef union {
  float f[3];
  unsigned char bytes[12];
} floatArr2Val;

floatArr2Val latlong;

float latitude;
float longitude;

int initialFramecounter = 271;
int framecounter;

int battery_status;

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

  // void setId(char *DevAddr, char *DevEUI, char *AppEUI);
  lora.setId("26011CC6", "00050197D45823E9", "70B3D57ED0009548");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey("22137B20EC10E8BEC32C11EBFB1681F0",
              "7D412196E418FA04CB8B1C080DCFC5C2", "NULL");

  lora.setDeviceMode(LWABP);
  lora.setAdaptiveDataRate(true);
  lora.setPower(MAX_EIRP_NDX_EU);
  lora.setReceiveWindowSecond(FREQ_RX_WNDW_SCND_EU, DOWNLINK_DATA_RATE_EU);

  // read frame counter from flash drive on boot
  framecounter = my_flash_store.read();
  if (initialFramecounter > framecounter) {
    my_flash_store.write(initialFramecounter);
    framecounter = initialFramecounter;
  }

  memset(buffer, 0, 256);
  lora.setCounters(buffer, 256, 1, framecounter, 0);
  SerialUSB.print(buffer);
}

void setup() {
  Serial.begin(9600);
  SerialUSB.begin(115200);

  // setup LoRa
  setupLoRaABP();

  pinMode(pin_battery_status, INPUT);

  // disable power to grove components
  digitalWrite(38, LOW);

  // set pin`s to output modules
  pinMode(GREENLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);

  digitalWrite(GREENLED, LOW);
  digitalWrite(BLUELED, HIGH);
}

void loop() {
  while (Serial.available() > 0) gps.encode(Serial.read());

  if (gps.location.isUpdated()) {
    //SerialUSB.println("++Got location update from GPS");
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
      if (firstUpdate != 1) SerialUSB.println("++Send First Lock Location");

      if (standStill == 1) {
        SerialUSB.println("++We are not driving");
      }

      trigger = 0;
      firstUpdate = 1;

      latlong.f[0] = latitude;
      latlong.f[1] = longitude;
      latlong.f[2] = gps.altitude.feet();

      unsigned long currentMillis = millis();

      if (millis() > updateTimeLORAsend - (LORAinterval / 10)) {
        digitalWrite(BLUELED, HIGH);
      }
      
      if (millis() > updateTimeLORAsend) {
        updateTimeLORAsend = millis() + LORAinterval;
        
        digitalWrite(BLUELED, LOW);

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
        framecounter++;
        memset(buffer, 0, 256);
        lora.getCounters(buffer, 256, 1);
        SerialUSB.print(buffer);
      }
    }
  } else {
    // update static location every 5 minutes
    if ((millis() > updateTimeLORAsendStandingStill) && (gps.location.lat() > 0)) {
        updateTimeLORAsendStandingStill = millis() + LORAintervalStandingStill;

        latlong.f[0] = gps.location.lat();;
        latlong.f[1] = gps.location.lng();;
        latlong.f[2] = gps.altitude.feet();
        
        digitalWrite(BLUELED, LOW);

        SerialUSB.print("++Static Location: ");
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
        framecounter++;
        memset(buffer, 0, 256);
        lora.getCounters(buffer, 256, 1);
        SerialUSB.print(buffer);
      }
  }
  
  if (gps.satellites.value() < 1) {
    int GPSlockdelay = 5000;
    if (millis() > updateTimeGPSlock) {
      // Blink Green Led until locked
      digitalWrite(GREENLED, LOW);
      // do some stuff, then redefine updateTime
      SerialUSB.println("++Waiting for GPS");
      SerialUSB.print("++Satellites ");
      SerialUSB.println(gps.satellites.value());
      SerialUSB.print("++Framecounter ");
      SerialUSB.println(framecounter);
      updateTimeGPSlock = millis() + GPSlockdelay;
    }
    if (millis() > updateTimeGPSlock - (GPSlockdelay / 2)) {
      // Turn leds off
      digitalWrite(GREENLED, HIGH);
      
    }
  } else {
    // we have lock keep green led on
    digitalWrite(GREENLED, LOW);
    digitalWrite(BLUELED, HIGH);
  }

  // check Battery
  if (hasBattery) {
    int BATTERYlockdelay = 600000;
    if (millis() > updateTimeBATTERYlock) {
      updateTimeBATTERYlock = millis() + BATTERYlockdelay;

      int a = analogRead(pin_battery_voltage);
      float v =
          a / 1023.0 * 3.3 * 11.0;  // there's an 1M and 100k resistor divider
      SerialUSB.print("++BATTERY ");
      SerialUSB.print(v, 2);
      SerialUSB.print("V low ");
      SerialUSB.print(bat_low, 2);
      SerialUSB.print("V high ");
      SerialUSB.print(bat_high, 2);
      SerialUSB.println(" V");
      battery_status = digitalRead(pin_battery_status);

      if (battery_status == 0) {
        SerialUSB.println("++BATTERY_STATUS Sleeping");
      }

      if (battery_status == 1) {
        SerialUSB.println("++BATTERY_STATUS Charging");
      }

      if (battery_status == 2) {
        SerialUSB.println("++BATTERY_STATUS Charging done");
      }

      if (battery_status == 3) {
        SerialUSB.println("++BATTERY_STATUS Error");
      }

      // check batery level when low ... go to deep sleep forever
      if (v > bat_low) {
        bat_low = v;
      }
      if (v > bat_high) {
        bat_high = v;
      }

      // Safe LiPo battery
      if (v <= 3.3 && battery_status == 0 && hasBattery) {
        nrgSave.begin(WAKE_EXT_INTERRUPT, 3,
                      dummy);  // write LoRaFrame before shutdown
        my_flash_store.write(framecounter);
        SerialUSB.println("MCU Stanby... Sleepy sleep in 10s from now ...");
        delay(10000);  // delay go to sleep
        digitalWrite(GREENLED, HIGH);
        digitalWrite(BLUELED, HIGH);
        digitalWrite(0, LOW);  // Rx
        digitalWrite(1, LOW);  // Tx
        digitalWrite(9, LOW);  // DTR  90uA
        pinMode(0, OUTPUT);
        pinMode(1, OUTPUT);
        Serial1.end();  // Disable Uart
        /*******************************************/
        nrgSave.standby();  // now mcu goes in standby mode
      }
    }
  }
}

void dummy(void)  // interrupt routine (isn't necessary to execute any tasks in
                  // this routine
{}
