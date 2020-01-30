#include <CanSatKit.h>
#include <cmath>
//#include <TinyMPU6050.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>

#define  MEAN_NUMBER  10
#define  MAX_PM   0
#define  MIN_PM   32767
int incomingByte = 0; // for incoming serial data
const int MAX_FRAME_LEN = 64;
char frameBuf[MAX_FRAME_LEN];
int detectOff = 0;
int frameLen = MAX_FRAME_LEN;
bool inFrame = false;
char printbuf[256];
unsigned int calcChecksum = 0;
unsigned int pm1_0=0, pm2_5=0, pm10_0=0;
unsigned int tmp_max_pm1_0, tmp_max_pm2_5, tmp_max_pm10_0; 
unsigned int tmp_min_pm1_0, tmp_min_pm2_5, tmp_min_pm10_0; 
byte i=0;

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

struct PMS7003_framestruct {
    byte  frameHeader[2];
    unsigned int  frameLen = MAX_FRAME_LEN;
    unsigned int  concPM1_0_CF1;
    unsigned int  concPM2_5_CF1;
    unsigned int  concPM10_0_CF1;
    unsigned int  checksum;
} thisFrame;


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

bool pms7003_read() {
#ifdef DEBUG  
    Serial.println("----- Reading PMS7003 -----");
#endif
    Serial1.begin(9600);
    bool packetReceived = false;
    calcChecksum = 0;
    while (!packetReceived) {
        if (Serial1 .available() > 32) {
            int drain = Serial1.available();
#ifdef DEBUG
                Serial.print("----- Draining buffer: -----");
                Serial.println(Serial1.available(), DEC);
#endif
            for (int i = drain; i > 0; i--) {
                Serial1.read();
            }
        }
        if (Serial1.available() > 0) {
#ifdef DEBUG
                Serial.print("----- Available: -----");
                Serial.println(Serial1.available(), DEC);
#endif
            incomingByte = Serial1.read();
#ifdef DEBUG
                Serial.print("----- READ: -----");
                Serial.println(incomingByte, HEX);
#endif
            if (!inFrame) {
                if (incomingByte == 0x42 && detectOff == 0) {
                    frameBuf[detectOff] = incomingByte;
                    thisFrame.frameHeader[0] = incomingByte;
                    calcChecksum = incomingByte; // Checksum init!
                    detectOff++;
                }
                else if (incomingByte == 0x4D && detectOff == 1) {
                    frameBuf[detectOff] = incomingByte;
                    thisFrame.frameHeader[1] = incomingByte;
                    calcChecksum += incomingByte;
                    inFrame = true;
                    detectOff++;
                }
                else {
                    Serial.print("----- Frame syncing... -----");
                    Serial.print(incomingByte, HEX);
                    Serial.println();
                }
            }
            else {
                frameBuf[detectOff] = incomingByte;
                calcChecksum += incomingByte;
                detectOff++;
                unsigned int  val = (frameBuf[detectOff-1]&0xff)+(frameBuf[detectOff-2]<<8);
                switch (detectOff) {
                    case 4:
                        thisFrame.frameLen = val;
                        frameLen = val + detectOff;
                        break;
                    case 6:
                        thisFrame.concPM1_0_CF1 = val;
                        break;
                    case 8:
                        thisFrame.concPM2_5_CF1 = val;
                        break;
                    case 10:
                        thisFrame.concPM10_0_CF1 = val;
                        break;
                    case 32:
                        thisFrame.checksum = val;
                        calcChecksum -= ((val>>8)+(val&0xFF));
                        break;
                    default:
                        break;
                }
                if (detectOff >= frameLen) {
#ifdef DEBUG          
                    sprintf(printbuf, "PMS7003 ");
                    sprintf(printbuf, "%s[%02x %02x] (%04x) ", printbuf,
                        thisFrame.frameHeader[0], thisFrame.frameHeader[1], thisFrame.frameLen);
                    sprintf(printbuf, "%sCF1=[%04x %04x %04x] ", printbuf,
                        thisFrame.concPM1_0_CF1, thisFrame.concPM2_5_CF1, thisFrame.concPM10_0_CF1);
                    sprintf(printbuf, "%scsum=%04x %s xsum=%04x", printbuf,
                        thisFrame.checksum, (calcChecksum == thisFrame.checksum ? "==" : "!="), calcChecksum);
                    Serial.println(printbuf);
#endif        
                    packetReceived = true;
                    detectOff = 0;
                    inFrame = false;
                }
            }
        }
    }
    Serial1.end();
    return (calcChecksum == thisFrame.checksum);
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

  if(i==0) { 
    tmp_max_pm1_0  = MAX_PM;
    tmp_max_pm2_5  = MAX_PM;
    tmp_max_pm10_0 = MAX_PM;
    tmp_min_pm1_0  = MIN_PM;
    tmp_min_pm2_5  = MIN_PM;
    tmp_min_pm10_0 = MIN_PM;
  }
  if (pms7003_read()) {
    tmp_max_pm1_0  = max(thisFrame.concPM1_0_CF1, tmp_max_pm1_0);
    tmp_max_pm2_5  = max(thisFrame.concPM2_5_CF1, tmp_max_pm2_5);
    tmp_max_pm10_0 = max(thisFrame.concPM10_0_CF1, tmp_max_pm10_0);
    tmp_min_pm1_0  = min(thisFrame.concPM1_0_CF1, tmp_min_pm1_0);
    tmp_min_pm2_5  = min(thisFrame.concPM2_5_CF1, tmp_min_pm2_5);
    tmp_min_pm10_0 = min(thisFrame.concPM10_0_CF1, tmp_min_pm10_0);
    pm1_0 += thisFrame.concPM1_0_CF1;
    pm2_5 += thisFrame.concPM2_5_CF1;
    pm10_0 += thisFrame.concPM10_0_CF1;
    i++;
  }
  pm1_0=pm2_5=pm10_0=i=0;
  
  if (!sd_active) {
    SerialUSB.println("SDCard is not working!");
  }

  // read PM
  packet.pmValid = pms7003_read();
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
  packet.pm10 = (pm1_0-tmp_max_pm1_0-tmp_min_pm1_0)/(MEAN_NUMBER-2);
  packet.pm25 = (pm2_5-tmp_max_pm2_5-tmp_min_pm2_5)/(MEAN_NUMBER-2);
  packet.pm100 = (pm10_0-tmp_max_pm10_0-tmp_min_pm10_0)/(MEAN_NUMBER-2);

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
