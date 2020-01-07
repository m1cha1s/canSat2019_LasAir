//#include <SDS011.h>
#include <SdsDustSensor.h>
#include <MPU6050_tockn.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <SD.h>
#include <SoftwareSerial.h>


const char *gpsStream =
  "$GPRMC,045103.000,A,3014.1984,N,09749.2872,W,0.67,161.46,030913,,,A*7C\r\n"
  "$GPGGA,045104.000,3014.1985,N,09749.2873,W,1,09,1.2,211.6,M,-22.5,M,,0000*62\r\n"
  "$GPRMC,045200.000,A,3014.3820,N,09748.9514,W,36.88,65.02,030913,,,A*77\r\n"
  "$GPGGA,045201.000,3014.3864,N,09748.9411,W,1,10,1.2,200.8,M,-22.5,M,,0000*6C\r\n"
  "$GPRMC,045251.000,A,3014.4275,N,09749.0626,W,0.51,217.94,030913,,,A*7D\r\n"
  "$GPGGA,045252.000,3014.4273,N,09749.0628,W,1,09,1.3,206.9,M,-22.5,M,,0000*6F\r\n";


// #define MYDEBUG 1

#define RFM95_CS 53
#define RFM95_RST 2
#define RFM95_INT 3

#define sdsRXpin 18
#define sdsTXpin 19

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);


BME280I2C bme;

int chipSel = 4;

MPU6050 imu(Wire);
SdsDustSensor mySDS(sdsRXpin, sdsTXpin);
TinyGPSPlus gps;

float p25, p10;
int err;

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSel)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("card initialized.");

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  delay(500);
  
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);

   Wire.begin();
  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }

  Wire.begin();
  imu.begin();
  imu.calcGyroOffsets(true);

  mySDS.begin();
}

int16_t packetnum = 0;  // packet counter, we increment per xmission


void loop()
{  
  float radiopacket[9];

  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);

  radiopacket[0], radiopacket[1] = pres, temp;

  PmResult pm = mySDS.readPm();
  if (pm.isOk()) {
    radiopacket[2] = pm.pm25;
    radiopacket[3] = pm.pm10;
  }
  imu.update();
  radiopacket[4] = imu.getGyroAngleX();
  radiopacket[5] = imu.getGyroAngleY();
  radiopacket[6] = imu.getGyroAngleZ();

  if(*gpsStream) {
    if(gps.encode(*gpsStream++)) {
      if(gps.location.isValid()) {
        radiopacket[7] = gps.location.lat();
        radiopacket[8] = gps.location.lng();
      }
    }
  }

  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)radiopacket, (sizeof(radiopacket)/sizeof(radiopacket[0]))*sizeof(float));
  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  //sendData(radiopacket);


  File logFile = SD.open("flightLog.txt", FILE_WRITE);

  if(logFile) {
    logFile.println(radiopacket[0]);
    logFile.println(radiopacket[1]);
    logFile.println(radiopacket[2]);
    logFile.println(radiopacket[3]);
    logFile.println(radiopacket[4]);
    logFile.println(radiopacket[5]);
    logFile.println(radiopacket[6]);
    logFile.println(radiopacket[7]);
    logFile.println(radiopacket[8]);
    logFile.println("--------------------");
    logFile.close();
  }  

#if defined(MYDEBUG)

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
    {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }

#endif


}