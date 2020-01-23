#include <CanSatKit.h>
#include <SD.h>
#include <SPI.h>
using namespace CanSatKit;

struct CanSatPacket {
  int id;
  unsigned long tm;
  int raw_temp;
  float temp1;
  float temp2;
  float pressure;
  float accX, accY, accZ, gyroX, gyroY, gyroZ, angleX, angleY, angleZ, accDeadzone, gyroDeadzone;
  float lng, lat, alt, speed;
  int satelites;
  bool satValid, altValid, locValid;
  float pm10, pm25, pm100;
  bool pmValid;
} packet;
const int packet_size = sizeof(packet);

File LOG;

int ledPin = 13;

// set radio receiver parameters - see comments below
// remember to set the same radio parameters in
// transmitter and receiver boards!
Radio radio(Pins::Radio::ChipSelect,
            Pins::Radio::DIO0,
            433.0,                  // frequency in MHz
            Bandwidth_125000_Hz,    // bandwidth - check with CanSat regulations to set allowed value
            SpreadingFactor_9,      // see provided presentations to determine which setting is the best
            CodingRate_4_8);        // see provided presentations to determine which setting is the best

void setup() {
  pinMode(ledPin, OUTPUT);
  SerialUSB.begin(115200);

  if (!SD.begin(11)) {
    SerialUSB.println("SD >> Card failed, or not present");
    while(1) {}
  }

  LOG = SD.open("recvLOG.txt", FILE_WRITE);

  // start radio module
  radio.begin();
}

void logDataF(char* title, float val) {
  SerialUSB.print(title);
  SerialUSB.print(val,10);
  //LOG.print(title);
  LOG.print(val,10);
  LOG.print(" ; ");
}

void logDataI(char* title, int val) {
  SerialUSB.print(title);
  SerialUSB.println(val, 10);
  //LOG.print(title);
  LOG.print(val, 10);
  LOG.print(" ; ");
}

void logDataB(char* title, bool val) {
  SerialUSB.print(title);
  SerialUSB.println(val, 10);
  //LOG.print(title);
  LOG.print(val, 10);
  LOG.print(" ; ");
}
void logAll() {
  logDataI("Packet ID: ", packet.id);
  logDataI("RSSI: ", radio.get_rssi_last());
  logDataI("Time: ", packet.tm);
  logDataI("Temp RAW: ", packet.raw_temp);
  logDataF("Temp1: ", packet.temp1);
  logDataF("Temp2: ", packet.temp2);
  logDataF("Pressure: ", packet.pressure);
  logDataF("AccX (m/s²): ", packet.accX);
  logDataF("AccY (m/s²): ", packet.accY);
  logDataF("AccZ (m/s²): ", packet.accZ);
  logDataF("GyroX (deg/s): ", packet.gyroX);
  logDataF("GyroY (deg/s): ", packet.gyroY);
  logDataF("GyroZ (deg/s): ", packet.gyroZ);
  logDataF("AngleX: ", packet.angleX);
  logDataF("AngleY: ", packet.angleY);
  logDataF("AngleZ: ", packet.angleZ);
  logDataF("AccDeadzone (m/s²): ", packet.accDeadzone);
  logDataF("gyroDeadzone (deg/s): ", packet.gyroDeadzone);
  logDataF("Lng: ", packet.lng);
  logDataF("Lat: ", packet.lat);
  logDataF("Speed: ", packet.speed);
  logDataF("Alt: ", packet.alt);
  logDataI("Satellites: ", packet.satelites);
  logDataB("SatValid: ", packet.satValid);
  logDataB("AltValid: ", packet.altValid);
  logDataB("LocValid: ", packet.locValid);
  logDataB("pmValid: ", packet.pmValid);
  logDataF("pm10: ", packet.pm10);
  logDataF("pm25: ", packet.pm25);
  logDataF("pm100: ",packet.pm100);
  SerialUSB.println("");
  LOG.println("");
}

void loop() {
  digitalWrite(ledPin, LOW);
  radio.receive((char *)(&packet));
  digitalWrite(ledPin, HIGH);
  
  logAll();

  LOG.flush();
}
