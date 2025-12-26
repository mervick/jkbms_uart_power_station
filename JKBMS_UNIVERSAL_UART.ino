#include "JKBMSInterface.h"

JKBMSInterface bms(&Serial1);

const int RELAY_PIN = 3; 
const float CHARGE_COMPLETE_THRESHOLD = 0.8f; 
float MAX_CURRENT = 0.0f;
bool IDLE = false;

void setup() {
  Serial.begin(115200); 
  bms.begin(115200); 
}

void loop() {
  if (IDLE) {
    delay(60000);
    return;
  }

  bms.update();

  if (bms.isDataValid()) {
    if (bms.isCharging()) {
      // Serial.println("Charging...");
      uint8_t soc = bms.getSOC();
      float current = bms.getCurrent() * -1; 

      if (soc == 100 && MAX_CURRENT > current && current <= CHARGE_COMPLETE_THRESHOLD) {
        pinMode(RELAY_PIN, OUTPUT);
        Serial.println("Charger OFF PERMANENTLY."); 
        IDLE = true;
        delay(1000);
        return;
      }

      if (current > MAX_CURRENT) {
        MAX_CURRENT = current;
      }
    }
  }

  delay(10000);
}
