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
  float accX, accY, accZ, gyroX, gyroY, gyroZ, accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ, angleX, angleY, angleZ;
  float lng, lat, alt, speed;
  int satelites;
  bool satValid, altValid, locValid;
  float um03, um05, um10, um25, um50, um100;
  bool pmValid;
} packet = {0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, false, false, false, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false};
int packet_size = sizeof(packet);

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

  // recvLOG = SD.open("recvLOG.txt", FILE_WRITE);

  // start radio module
  radio.begin();
}

void logDataF(char* title, float val) {
  SerialUSB.print(title);
  SerialUSB.println(val);
  LOG.print(title);
  LOG.println(val);
}

void logDataI(char* title, int val) {
  SerialUSB.print(title);
  SerialUSB.println(val);
  LOG.print(title);
  LOG.println(val);
}

void logDataB(char* title, bool val) {
  SerialUSB.print(title);
  SerialUSB.println(val);
  LOG.print(title);
  LOG.println(val);
}

void logAll() {
  logDataI("Packet ID: ", packet.id);
  logDataI("RSSI: ", radio.get_rssi_last());
  logDataI("Time: ", packet.tm);
  logDataF("Temp RAW: ", packet.raw_temp);
  logDataF("Temp1: ", packet.temp1);
  logDataF("Temp2: ", packet.temp2);
  logDataF("Pressure: ", packet.pressure);
  logDataF("AccX: ", packet.accX);
  logDataF("AccY: ", packet.accY);
  logDataF("AccZ: ", packet.accZ);
  logDataF("GyroX: ", packet.gyroX);
  logDataF("GyroY: ", packet.gyroY);
  logDataF("GyroZ: ", packet.gyroZ);
  logDataF("AccAngleX: ", packet.accAngleX);
  logDataF("AccAngleY: ", packet.accAngleY);
  logDataF("GyroAngleX: ", packet.gyroAngleX);
  logDataF("GyroAngleY: ", packet.gyroAngleY);
  logDataF("GyroAngleZ: ", packet.gyroAngleZ);
  logDataF("AngleX: ", packet.angleX);
  logDataF("AngleY: ", packet.angleY);
  logDataF("AngleZ: ", packet.angleZ);
  logDataF("Lng: ", packet.lng);
  logDataF("Lat: ", packet.lat);
  logDataF("Speed: ", packet.speed);
  logDataF("Alt: ", packet.alt);
  logDataI("Satellites: ", packet.satelites);
  logDataB("SatValid: ", packet.satValid);
  logDataB("AltValid: ", packet.altValid);
  logDataB("LocValid: ", packet.locValid);
  logDataF("0.3 um: ", packet.um03);
  logDataF("0.5 um: ", packet.um05);
  logDataF("10 um: ", packet.um10);
  logDataF("25 um: ", packet.um25);
  logDataF("50 um: ", packet.um50);
  logDataF("100 um: ", packet.um100);
  logDataB("pmValid: ", packet.pmValid);
  SerialUSB.println("");
  LOG.println("");
}



void loop() {
  LOG = SD.open("recvLOG.txt", FILE_WRITE);

  radio.receive((char *)(&packet));
  digitalWrite(ledPin, HIGH);

  logAll();

  LOG.close();
  digitalWrite(ledPin, LOW);

}
