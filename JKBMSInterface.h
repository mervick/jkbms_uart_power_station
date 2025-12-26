#ifndef JKBMSInterface_H
#define JKBMSInterface_H

#include <Arduino.h>

class JKBMSInterface {
public:
    // Constructor
    JKBMSInterface(HardwareSerial* serial);

    // Initialize the BMS communication
    void begin(uint32_t baudRate = 115200);

    // Main update function - call this in your loop()
    void update();

    // Request data from BMS (automatically called by update())
    void requestData();

    // Basic data getters (0x83-0x87)
    float getVoltage();                    // 0x83 Total voltage
    float getCurrent();                    // 0x84 Current
    uint8_t getSOC();                      // 0x85 State of charge
    uint8_t getNumTempSensors();           // 0x86 Number of temperature sensors
    uint16_t getCycles();                  // 0x87 Battery cycles

    // Advanced data getters (0x89-0x8A)
    uint32_t getTotalCycleCapacity();      // 0x89 Total cycle capacity (Ah)
    uint16_t getTotalBatteryStrings();     // 0x8A Total number of battery strings

    // Temperature getters (0x80-0x82)
    float getPowerTemp();                  // 0x80 Power tube temperature
    float getBoxTemp();                    // 0x81 Battery box temperature
    float getBatteryTemp();                // 0x82 Battery temperature

    // Cell voltage getters (0x79)
    uint8_t getNumCells();
    float getCellVoltage(uint8_t cellIndex);
    float getLowestCellVoltage();
    float getHighestCellVoltage();
    float getCellVoltageDelta();

    bool isCharging();
    bool isDischarging();

    // Data validity
    bool isDataValid();

    // Display functions
    void printSummary();
    void printRawData();

private:
    float _getCurrent();
    struct BMSData {
        // Cell voltages (0x79)
        float cellVoltages[24];
        uint8_t numCells;

        // Basic measurements (0x80-0x87)
        float powerTemp;
        float boxTemp;
        float batteryTemp;

        float totalVoltage;
        uint16_t current;
        uint8_t soc;

        uint8_t numTempSensors;
        uint16_t cycles;

        // Advanced data (0x89-0x8C)
        uint32_t totalCycleCapacity;
        uint16_t totalBatteryStrings;

        uint8_t protocolVersion;

        bool dataValid;
    };

    HardwareSerial* _serial;
    BMSData _bmsData;
    uint8_t _responseBuffer[512];
    uint16_t _responseIndex;
    unsigned long _lastCommandSent;

    // Private parsing methods
    void parseRawData(uint8_t* data, int length);
    void clearData();
};

#endif
