#include "JKBMSInterface.h"

JKBMSInterface bms(&Serial1);

const int RELAY_PIN = 3; 
const float CHARGE_COMPLETE_THRESHOLD = 0.8f; 
float MAX_CURRENT = 0.0f;
bool IDLE = false;

void setup() {
  Serial.begin(115200); 
  bms.begin(115200); 
  // Serial.println("Arduino Pro Micro Started!");

  // Изначально реле выключено (зарядное устройство включено)
  // pinMode(RELAY_PIN, OUTPUT);
  // digitalWrite(RELAY_PIN, LOW); 
  // pinMode(RELAY_PIN, INPUT);
}

void loop() {
  if (IDLE) {
    delay(60000);
    return;
  }

  bms.update();

  if (bms.isDataValid()) {
    // bms.printSummary();
    if (bms.isCharging()) { // Нас интересует только ток заряда
      // Serial.println("Charging...");
      uint8_t soc = bms.getSOC();
      float current = bms.getCurrent() * -1; 

      // Serial.println("RELAY_PIN HIGH");
      // pinMode(RELAY_PIN, OUTPUT);
      // digitalWrite(RELAY_PIN, HIGH); 
      // Serial.println(MAX_CURRENT);
      // Serial.println(current);

      // Если ток упал ниже порогового значения
      if (soc == 100 && MAX_CURRENT > current && current <= CHARGE_COMPLETE_THRESHOLD) {
        // !!! Действие: Отключаем зарядное устройство !!!
        // Serial.println("Turn ON Relay");
        Serial.println("RELAY_PIN HIGH");
        pinMode(RELAY_PIN, OUTPUT);
        // digitalWrite(RELAY_PIN, HIGH); // Включаем реле
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

  // digitalWrite(RELAY_PIN, LOW); 
  delay(10000);
}