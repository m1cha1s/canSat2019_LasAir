#include <spi.h>
#include <lora.h>


void setup() {
    Serial.begin(9600);

    while(!Serial);


    while(!LoRa.begin(433000000)) {
        Serial.println("Waiting for lora to start ...");
        delay(100);
    }
}

void loop() {
    LoRa.beginPacket();
    LoRa.print("Hello, world");
    LoRa.endPacket();

    while(LoRa.isTransmitting()) {
        delay(1);
    }

}