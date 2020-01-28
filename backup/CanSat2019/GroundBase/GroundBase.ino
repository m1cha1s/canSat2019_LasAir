#include <CanSatKit.h>
#include <SPI.h>
#include <SD.h>
using namespace CanSatKit;

struct CanSatPacket {
  int id;
  float tm;
  float temp1;
  float temp2;
  float pressure;
  //float accX, accY, accZ, gyroX, gyroY, gyroZ, angleX, angleY, angleZ, accDeadzone, gyroDeadzone;
  float lng, lat, alt, speed;
  int satelites;
  bool satValid, altValid, locValid;
  float pm10, pm25, pm100;
  bool pmValid;
} packet;
const int packet_size = sizeof(packet);

File LOG;
int sd_active;

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

  // delay(5000);

  sd_active = 1;
  if (!SD.begin(11)) {
    sd_active = 0;
  }

  if (sd_active) {
    LOG = SD.open("recvLOG.txt", FILE_WRITE);
  }

  // start radio module
  radio.begin();
}

void logDataF(char* title, float val) {
  SerialUSB.print(title);
  SerialUSB.println(val, 10);
  if (sd_active) {
    LOG.print(val, 10);
    LOG.print(";");
  }
}

void logDataI(char* title, int val) {
  SerialUSB.print(title);
  SerialUSB.println(val);
  if (sd_active) {
    LOG.print(val);
    LOG.print(";");
  }
}

void logDataB(char* title, bool val) {
  SerialUSB.print(title);
  SerialUSB.println(val);
  if (sd_active) {
    LOG.print(val);
    LOG.print(";");
  }
}
void logAll() {
  logDataI("Packet ID: ", packet.id);
  logDataI("RSSI: ", radio.get_rssi_last());
  logDataF("Time: ", packet.tm);
  logDataF("Temp1: ", packet.temp1);
  logDataF("Temp2: ", packet.temp2);
  logDataF("Pressure: ", packet.pressure);
  //logDataF("AccX (m/s²): ", packet.accX);
  //logDataF("AccY (m/s²): ", packet.accY);
  //logDataF("AccZ (m/s²): ", packet.accZ);
  //logDataF("GyroX (deg/s): ", packet.gyroX);
  //logDataF("GyroY (deg/s): ", packet.gyroY);
  //logDataF("GyroZ (deg/s): ", packet.gyroZ);
  //logDataF("AngleX: ", packet.angleX);
  ///logDataF("AngleY: ", packet.angleY);
  //logDataF("AngleZ: ", packet.angleZ);
  //logDataF("AccDeadzone (m/s²): ", packet.accDeadzone);
  //logDataF("gyroDeadzone (deg/s): ", packet.gyroDeadzone);
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
  logDataF("pm100: ", packet.pm100);
  SerialUSB.println();
  if (sd_active) {
    LOG.println();
  }
}

void loop() {
  if (!sd_active) {
    SerialUSB.println("SDCard is not working!");
  }

  digitalWrite(ledPin, HIGH);
  radio.receive((char *)(&packet));
  digitalWrite(ledPin, LOW);

  logAll();

  if (sd_active) {
    LOG.flush();
  }
}
