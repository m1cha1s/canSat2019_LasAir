#include <CanSatKit.h>
#include <cmath>
//#include <TinyMPU6050.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>


using namespace CanSatKit;
//14
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

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;

const int lm35_pin = A0;

float lm35_raw_to_temperature(int raw) {
  float voltage = raw * 3.3 / (std::pow(2, 12));
  float temperature = 100.0 * voltage;
  return temperature;
}

//MPU6050 mpu6050(Wire);
TinyGPSPlus gps;
BMP280 bmp;

Radio radio(Pins::Radio::ChipSelect,
            Pins::Radio::DIO0,
            433.0,
            Bandwidth_125000_Hz,
            SpreadingFactor_9,
            CodingRate_4_8);

File LOG;
int sd_active;

void setup() {
  packet.id = 0;
  packet.tm = 0;
  Serial.begin(9600);
  Serial1.begin(9600);
  Wire.begin();
  //mpu6050.Initialize();
  //mpu6050.Calibrate();
  bmp.begin();
  bmp.setOversampling(16);
  radio.begin();
  analogReadResolution(12);

  sd_active = 1;
  if (!SD.begin(11)) {
    sd_active = 0;
  }

  if (sd_active) {
    LOG = SD.open("dataLOG.txt", FILE_WRITE);
  }
}

boolean readPMSdata() {
  if (!Serial1.available()) {
    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (Serial1.peek() != 0x42) {
    Serial1.read();
    return false;
  }

  // Now read all 32 bytes
  if (Serial1.available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  Serial1.readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  /* debugging
    for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
    }
    Serial.println();
  */

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);

  if (sum != data.checksum) {
    SerialUSB.println("Checksum failure");
    return false;
  }
  // success!
  return true;
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

void logDataB(char * title, bool val) {
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

void readGPSData() {
  while (Serial.available()) {
    char c = Serial.read();
    gps.encode(c);
    //  SerialUSB.print(c);
  }
}

void loop() {
  unsigned long t1 = millis();

  if (!sd_active) {
    SerialUSB.println("SDCard is not working!");
  }

  // read PM
  packet.pmValid = readPMSdata();
  // read gyro
  //mpu6050.Execute();

  // read analog termometer
  double t, p;
  bmp.measureTemperatureAndPressure(t, p);
  int raw_temp = analogRead(lm35_pin);
  float temp = lm35_raw_to_temperature(raw_temp);

  // read GPS
  readGPSData();

  packet.id++;
  packet.tm = millis() / 1000.0;
  packet.temp1 = temp;
  packet.temp2 = t;
  packet.pressure = p;
  //packet.accX = mpu6050.GetAccX();
  //packet.accY = mpu6050.GetAccY();
  //packet.accZ = mpu6050.GetAccZ();
  //packet.gyroX = mpu6050.GetGyroX();
  //packet.gyroY = mpu6050.GetGyroY();
  //packet.gyroZ = mpu6050.GetGyroZ();
  //packet.angleX = mpu6050.GetAngX();
  //packet.angleY = mpu6050.GetAngY();
  //packet.angleZ = mpu6050.GetAngZ();
  //packet.accDeadzone = mpu6050.GetAccelDeadzone();
  //packet.gyroDeadzone = mpu6050.GetGyroDeadzone();
  packet.lng = gps.location.lng();
  packet.lat = gps.location.lat();
  packet.speed = gps.speed.kmph();
  packet.alt = gps.altitude.meters();
  packet.satelites = gps.satellites.value();
  packet.satValid = gps.satellites.isValid();
  packet.altValid = gps.altitude.isValid();
  packet.locValid = gps.location.isValid();
  packet.pm10 = data.pm10_standard;
  packet.pm25 = data.pm25_standard;
  packet.pm100 = data.pm100_standard;

  // packet_size = 120 bytes
  // transmission takes 1250ms
  radio.transmit((uint8_t *)(&packet), packet_size);
  radio.flush();

  logAll();
  if (sd_active) {
    LOG.flush();
  }

  unsigned long tdiff = millis() - t1;

  SerialUSB.print("Timediff: ");
  SerialUSB.println(tdiff);
  SerialUSB.println(packet_size);

  if (tdiff <= 250) delay(250 - tdiff);
}
