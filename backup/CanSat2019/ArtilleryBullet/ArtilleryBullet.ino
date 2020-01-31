#include <CanSatKit.h>
#include <cmath>
//#include <TinyMPU6050.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <Plantower_PMS7003.h>

//int sleepModePMS = 2;

char output[256];
Plantower_PMS7003 pms7003 = Plantower_PMS7003();

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
  SerialUSB.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
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
  pms7003.init(&Serial1);

  sd_active = 1;
  if (!SD.begin(11)) {
    sd_active = 0;
  }

  if (sd_active) {
    LOG = SD.open("dataLOG.txt", FILE_WRITE);
  }
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
  pms7003.updateFrame();
  digitalWrite(13, LOW);
  unsigned long t1 = millis();
  
  if (!sd_active) {
    SerialUSB.println("SDCard is not working!");
  }
  else {
    SerialUSB.println("SDCard is working!");
  }

  // read PM
  packet.pmValid = pms7003.hasNewData();
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
  packet.pm10 = pms7003.getPM_1_0_atmos();
  packet.pm25 = pms7003.getPM_2_5_atmos();
  packet.pm100 = pms7003.getPM_10_0_atmos();
  

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
  digitalWrite(13, HIGH);
}
