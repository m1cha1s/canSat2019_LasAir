#include <MPU6050_tockn.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <SD.h>
#include <SoftwareSerial.h>


// #define MYDEBUG 1

#define RFM95_CS 53
#define RFM95_RST 2
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

BME280I2C bme;

int chipSel = 4;

MPU6050 imu(Wire);

void setup()
{
  serialINIT();
  loraINIT();
  bmeINIT();
  sdINIT(chipSel);
  mpu6050INIT();
}

int16_t packetnum = 0;  // packet counter, we increment per xmission


void loop()
{
  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  delay(1000); // Wait 1 second between transmits, could also 'sleep' here!

  bme.read(pres, temp, hum, tempUnit, presUnit);

  float radiopacket[2];
  radiopacket[0] = temp;
  radiopacket[1] = pres;

  Serial.println(sizeof(float));
  Serial.println(temp);
  Serial.println(pres);


  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)radiopacket, 2*sizeof(float));

  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

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

void logData(String dataString) {
  File logFile = SD.open("flightLog.txt", FILE_WRITE);

  if(logFile) {
    logFile.println(dataString);
    logFile.close();
  }  
}

void sdINIT(int chipSelect) {
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("card initialized.");
}

void loraINIT() {
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
}

void bmeINIT() {
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
}

void serialINIT() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }

  delay(100);  
}

void mpu6050INIT() {
  Wire.begin();
  imu.begin();
  imu.calcGyroOffsets(true);
}