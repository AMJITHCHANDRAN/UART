#include <EEPROM.h>

const int BAUD_RATE = 2400;
const int EEPROM_SIZE = 1024;
const int CHUNK_SIZE = 32;  // Chunk size for receiving/transmitting data

void setup() {
  Serial.begin(BAUD_RATE);
}

void loop() {
  receiveData();
  transmitData();
}

void receiveData() {
  static int dataIndex = 0;
  while (Serial.available()) {
    EEPROM.write(dataIndex++, Serial.read());
    delay(10);  // Add delay to allow EEPROM write
  }
}

void transmitData() {
  static int dataIndex = 0;
  while (dataIndex < EEPROM_SIZE) {
    Serial.write(EEPROM.read(dataIndex++));
    delay(10);  // Add delay for transmission
  }
}