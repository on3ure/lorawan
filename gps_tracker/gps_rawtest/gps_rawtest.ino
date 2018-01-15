#include <LoRaWan.h>

// what's the name of the hardware serial port?
#define GPSSerial Serial

void setup() {
  // wait for hardware serial to appear
  while (!SerialUSB);

  // make this baud rate fast enough to we aren't waiting on it
  SerialUSB.begin(115200);

  // 9600 baud is the default rate for the GPS
  GPSSerial.begin(9600);
}

     
void loop() {
  if (SerialUSB.available()) {
    char c = SerialUSB.read();
    GPSSerial.write(c);
  }
  if (GPSSerial.available()) {
    char c = GPSSerial.read();
    SerialUSB.write(c);
  }
}
