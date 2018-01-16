#include <TinyGPS++.h>
#include <TimerTCC0.h>
#include<LoRaWan.h>

#define FREQ_RX_WNDW_SCND_EU  869.525
const float EU_hybrid_channels[8] = {868.1, 868.3, 868.5, 867.1, 867.3, 867.5, 867.7, 867.9}; //rx 869.525
#define DOWNLINK_DATA_RATE_EU DR8
#define EU_RX_DR DR8
#define UPLINK_DATA_RATE_MAX_EU  DR5
#define MAX_EIRP_NDX_EU  2
#define UPLINK_DATA_RATE_MIN DR0
#define DEFAULT_RESPONSE_TIMEOUT 5

/*
   This sample sketch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
char buffer[128] = {0};
int sec = 0;

// The TinyGPS++ object
TinyGPSPlus gps;
typedef union {
    float f[2];         // Assigning fVal.f will also populate fVal.bytes;
    unsigned char bytes[8];   // Both fVal.f and fVal.bytes share the same 4 bytes of memory.
} floatArr2Val;

floatArr2Val latlong;

float latitude;
float longitude;

void setHybridForTTN(const float* channels){
    for(int i = 0; i < 8; i++){
        if(channels[i] != 0){
            lora.setChannel(i, channels[i], UPLINK_DATA_RATE_MIN, UPLINK_DATA_RATE_MAX_EU);        
        }
    }
}

void displayInfo()
{
    if(latitude){
      SerialUSB.println("Got real lat");
    }
    if(longitude){
      SerialUSB.println("Got real lon");
    }
    SerialUSB.print(F("Location: ")); 
    SerialUSB.print(latlong.f[0], 6);
    //SerialUSB.print(latitude, 6);
    SerialUSB.print(F(","));
    SerialUSB.print(latlong.f[1], 6);
    //SerialUSB.print(longitude, 6);
    SerialUSB.println();
}

void timerIsr(void)  //interrupt routine
{
    sec = (sec + 1) % 6;   
    SerialUSB.println(sec);
    displayInfo();
}

void setupLoRaABP(){  
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
  lora.setId("26011AE4", "00F33FDD11542C5B", "70B3D57ED00094C3");
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey("0B74363CDAF7DFE59EB49A8CBD5FE13D", "1C0E3362476E45486A82DF26328DB510", "3AFE4E0D4A5EE9534613B7E147C5DF20");
    
    lora.setDeciveMode(LWABP);
    lora.setDataRate(DR0, EU868);
    //lora.setPower(MAX_EIRP_NDX_EU);   
    lora.setPower(14);
    setHybridForTTN(EU_hybrid_channels);
    //lora.setReceiceWindowFirst(1);
    lora.setReceiceWindowFirst(0,  EU_hybrid_channels[0]);
    lora.setReceiceWindowSecond(FREQ_RX_WNDW_SCND_EU, DOWNLINK_DATA_RATE_EU);

    
}

void setup()
{
    Serial.begin(9600);
    SerialUSB.begin(115200);
    
    memset(buffer, 0, 256);
    setupLoRaABP();
    //setupLoRaOTAA();
  //TimerTcc0.initialize(60000000); 1 Minute
    TimerTcc0.initialize(10000000); //10 seconds
    TimerTcc0.attachInterrupt(timerIsr);
}

void loop()
{
  
//    if (sec == 3){
//      //Serial.write("h"); //Turn on GPS
//    }
    if (sec <= 2 ) {
        while (Serial.available() > 0){
            char currChar = Serial.read();
            //SerialUSB.print(currChar);
            gps.encode(currChar);
        }
        latitude  = gps.location.lat();
        longitude = gps.location.lng();
        //if((latitude && longitude) && latitude != latlong.f[0]
        //    && longitude != latlong.f[1]){     
            latlong.f[0] = latitude;
            latlong.f[1] = longitude;
            
            SerialUSB.println("LatLong: ");
            for(int i = 0; i < 8; i++){
                SerialUSB.print(latlong.bytes[i], HEX);
            }
            SerialUSB.println("");
            bool result = lora.transferPacket(latlong.bytes, 8, DEFAULT_RESPONSE_TIMEOUT);
        //}
    }
//    else if (sec == 5 ){
//        //Serial.write("$PMTK161,0*28\r\n"); //Put GPS to sleep
//    }
}
