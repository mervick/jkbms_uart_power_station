#include "JKBMSInterface.h"

// Read all command from protocol documentation
static const uint8_t readAllCommand[] = {
    0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x68, 0x00, 0x00, 0x01, 0x29
};

JKBMSInterface::JKBMSInterface(HardwareSerial* serial)
    : _serial(serial), _responseIndex(0), _lastCommandSent(0) {
    clearData();
}

void JKBMSInterface::begin(uint32_t baudRate) {
    _serial->begin(baudRate, SERIAL_8N1);
    clearData();

    while (_serial->available()) {
        _serial->read();
    }
}

void JKBMSInterface::clearData() {
    _bmsData.dataValid = false;
    _bmsData.numCells = 0;
    _bmsData.totalVoltage = 0;
    _bmsData.current = 0;
    _bmsData.soc = 0;
    _bmsData.powerTemp = 0;
    _bmsData.boxTemp = 0;
    _bmsData.batteryTemp = 0;
    _bmsData.numTempSensors = 0;
    _bmsData.cycles = 0;
    _bmsData.totalCycleCapacity = 0;
    _bmsData.totalBatteryStrings = 0;

    _bmsData.protocolVersion = 1;


    for (int i = 0; i < 24; i++) {
        _bmsData.cellVoltages[i] = 0;
    }
}

void JKBMSInterface::update() {
    if (millis() - _lastCommandSent > 5000) {
        requestData();
    }
    _responseIndex = 0;

    while (_serial->available()) {
        uint8_t byte = _serial->read();
       _responseBuffer[_responseIndex++] = byte;

        if (_responseIndex >= 4 &&
            _responseBuffer[0] == 0x4e &&
            _responseBuffer[1] == 0x57) {

            parseRawData(_responseBuffer, _responseIndex);
        }

        if (_responseIndex >= 512) {
            _responseIndex = 0;
        }
    }
}

void JKBMSInterface::requestData() {
    _serial->write(readAllCommand, sizeof(readAllCommand));
    _lastCommandSent = millis();
    _responseIndex = 0;
}

void JKBMSInterface::parseRawData(uint8_t* data, int length) {
    _bmsData.dataValid = false;
    _bmsData.numCells = 0;

    int pos = 11; // Skip header

    while (pos < length) {
        if (pos >= length) break;

        uint8_t dataId = data[pos++];

        switch (dataId) {
            case 0x79: // Cell voltages
                {
                    if (pos < length) {
                        uint8_t dataLength = data[pos++];
                        int endPos = pos + dataLength;
                        _bmsData.numCells = 0;

                        while (pos + 3 <= endPos && pos + 3 <= length) {
                            uint8_t cellNum = data[pos++];
                            uint16_t voltage = (data[pos] << 8) | data[pos+1];
                            pos += 2;

                            if (cellNum > 0 && cellNum <= 24 && voltage > 0) {
                                _bmsData.cellVoltages[cellNum-1] = voltage / 1000.0f;
                                _bmsData.numCells = max(_bmsData.numCells, cellNum);
                            }
                        }
                    }
                }
                break;

            case 0x80: // Power tube temperature
                if (pos + 1 < length) {
                    uint16_t temp = (data[pos] << 8) | data[pos+1];
                    _bmsData.powerTemp = (temp <= 100) ? temp : -(temp - 100);
                    pos += 2;
                }
                break;

            case 0x81: // Box temperature
                if (pos + 1 < length) {
                    uint16_t temp = (data[pos] << 8) | data[pos+1];
                    _bmsData.boxTemp = (temp <= 100) ? temp : -(temp - 100);
                    pos += 2;
                }
                break;

            case 0x82: // Battery temperature
                if (pos + 1 < length) {
                    uint16_t temp = (data[pos] << 8) | data[pos+1];
                    _bmsData.batteryTemp = (temp <= 100) ? temp : -(temp - 100);
                    pos += 2;
                }
                break;

            case 0x83: // Total voltage
                if (pos + 1 < length) {
                    uint16_t voltage = (data[pos] << 8) | data[pos+1];
                    _bmsData.totalVoltage = voltage * 0.01f;
                    pos += 2;
                }
                break;

            case 0x84: // Current
                if (pos + 1 < length) {
                    _bmsData.current = (data[pos] << 8) | data[pos+1];
                    pos += 2;
                }
                break;

            case 0x85: // SOC
                if (pos < length) {
                    _bmsData.soc = data[pos++];
                }
                break;

            case 0x86: // Number of temperature sensors
                if (pos < length) {
                    _bmsData.numTempSensors = data[pos++];
                }
                break;

            case 0x87: // Cycles
                if (pos + 1 < length) {
                    _bmsData.cycles = (data[pos] << 8) | data[pos+1];
                    pos += 2;
                }
                break;

            case 0x89: // Total cycle capacity
                if (pos + 3 < length) {
                    _bmsData.totalCycleCapacity = ((uint32_t)data[pos] << 24) |
                                                  ((uint32_t)data[pos+1] << 16) |
                                                  ((uint32_t)data[pos+2] << 8) |
                                                  data[pos+3];
                    pos += 4;
                }
                break;

            case 0x8a: // Total battery strings
                if (pos + 1 < length) {
                    _bmsData.totalBatteryStrings = (data[pos] << 8) | data[pos+1];
                    pos += 2;
                }
                break;

            case 0x8b:
            case 0x8c:
            case 0x8e:
            case 0x8f:
            case 0x90:
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x96:
            case 0x97:
            case 0x98:
            case 0x99:
                _bmsData.dataValid = true;
                return;

            case 0xc0: // Protocol version
                if (pos < length) {
                    _bmsData.protocolVersion = data[pos++];
                }
                break;

            case 0x68: // End marker
                _bmsData.dataValid = true;
                return;


            default:
                // Skip unknown data types
                if (dataId >= 0x8a && dataId <= 0xC0) {
                    pos += 2;
                } else {
                    pos += 1;
                }
                break;
        }
    }

    _bmsData.dataValid = true;
}

// Basic getters
float JKBMSInterface::getVoltage() {
    return _bmsData.dataValid ? _bmsData.totalVoltage : -1.0f;
}

float JKBMSInterface::_getCurrent() {
    uint16_t current = _bmsData.current;
    float curr = 0.0f;
    const float CURRENT_UNIT = 0.01f;

    if (_bmsData.protocolVersion == 0x01) {
        // Protocol version 0x01: highest bit indicates charge/discharge
        // if (current & 0x8000) {
        //     // Charging (bit 15 = 1)
        //     curr = -(current & 0x7FFF) * 0.01f;
        // } else {
        //     // Discharging (bit 15 = 0)
        //     curr = current * 0.01f;
        // }

        // Маска для получения абсолютной величины тока (15 младших бит)
        uint16_t magnitude = current & 0x7FFF;

        if (current & 0x8000) {
            // Charging (bit 15 = 1). In standard convention, Charging is POSITIVE.
            curr = -(float)magnitude * CURRENT_UNIT;
        } else {
            // Discharging (bit 15 = 0). In standard convention, Discharging is NEGATIVE.
            curr = (float)magnitude * CURRENT_UNIT;
        }
    } else {
        // Default protocol version 0x00
        if (current == 0 || current == 10000) {
            curr = 0.0f;
        } else if (current > 10000) {
            curr = (current - 10000) * 0.01f;
        } else {
            curr = -(10000 - current) * 0.01f;
        }
    }

    return curr;
}

float JKBMSInterface::getCurrent() {
    return _bmsData.dataValid ? _getCurrent() : 0.0f;
}

uint8_t JKBMSInterface::getSOC() {
    return _bmsData.dataValid ? _bmsData.soc : 0;
}

uint8_t JKBMSInterface::getNumTempSensors() {
    return _bmsData.dataValid ? _bmsData.numTempSensors : 0;
}

uint16_t JKBMSInterface::getCycles() {
    return _bmsData.dataValid ? _bmsData.cycles : 0;
}

uint32_t JKBMSInterface::getTotalCycleCapacity() {
    return _bmsData.dataValid ? _bmsData.totalCycleCapacity : 0;
}

uint16_t JKBMSInterface::getTotalBatteryStrings() {
    return _bmsData.dataValid ? _bmsData.totalBatteryStrings : 0;
}

// Temperature getters
float JKBMSInterface::getPowerTemp() {
    return _bmsData.dataValid ? _bmsData.powerTemp : -999.0f;
}

float JKBMSInterface::getBoxTemp() {
    return _bmsData.dataValid ? _bmsData.boxTemp : -999.0f;
}

float JKBMSInterface::getBatteryTemp() {
    return _bmsData.dataValid ? _bmsData.batteryTemp : -999.0f;
}

// Cell voltage getters
uint8_t JKBMSInterface::getNumCells() {
    return _bmsData.dataValid ? _bmsData.numCells : 0;
}

float JKBMSInterface::getCellVoltage(uint8_t cellIndex) {
    if (!_bmsData.dataValid || cellIndex >= 24 || cellIndex >= _bmsData.numCells) {
        return -1.0f;
    }
    return _bmsData.cellVoltages[cellIndex];
}

float JKBMSInterface::getLowestCellVoltage() {
    if (!_bmsData.dataValid || _bmsData.numCells == 0) return -1.0f;

    float lowest = _bmsData.cellVoltages[0];
    for (int i = 1; i < _bmsData.numCells; i++) {
        if (_bmsData.cellVoltages[i] > 0 && _bmsData.cellVoltages[i] < lowest) {
            lowest = _bmsData.cellVoltages[i];
        }
    }
    return lowest;
}

float JKBMSInterface::getHighestCellVoltage() {
    if (!_bmsData.dataValid || _bmsData.numCells == 0) return -1.0f;

    float highest = _bmsData.cellVoltages[0];
    for (int i = 1; i < _bmsData.numCells; i++) {
        if (_bmsData.cellVoltages[i] > highest) {
            highest = _bmsData.cellVoltages[i];
        }
    }
    return highest;
}

float JKBMSInterface::getCellVoltageDelta() {
    if (!_bmsData.dataValid || _bmsData.numCells == 0) return -1.0f;
    return getHighestCellVoltage() - getLowestCellVoltage();
}


bool JKBMSInterface::isCharging() {
    float current = getCurrent();
    return _bmsData.dataValid ? current < -0.01f : false;
}

bool JKBMSInterface::isDischarging() {
    float current = getCurrent();
    return _bmsData.dataValid ? current > 0.01f : false;
}


bool JKBMSInterface::isDataValid() {
    return _bmsData.dataValid;
}

// Display functions
void JKBMSInterface::printSummary() {
    if (!_bmsData.dataValid) {
        Serial.println("No valid BMS data available");
        return;
    }
    Serial.print("Voltage: ");
    Serial.print(_bmsData.totalVoltage, 2);
    Serial.print("V  SOC: ");
    Serial.print(_bmsData.soc);
    Serial.println("%");

    float current = getCurrent();

    Serial.print("Current: ");
    if (current > 0.01f) {
        Serial.print(current, 2);
        Serial.println("A (Discharging)");
    } else if (current < -0.01f) {
        Serial.print(-current, 2);
        Serial.println("A (Charging)");
    } else {
        Serial.println("0.00A (Idle)");
    }

    Serial.print("Temps: Power=");
    Serial.print(_bmsData.powerTemp, 1);
    Serial.print("°C Battery=");
    Serial.print(_bmsData.batteryTemp, 1);
    Serial.println("°C");

    Serial.print("Cycles: ");
    Serial.print(_bmsData.cycles);
    Serial.print("  Cells: ");
    Serial.print(_bmsData.numCells);
    Serial.println("");
}

void JKBMSInterface::printRawData() {
    for (int i = 0; i < min(_responseIndex, 512); i++) {
        if (_responseBuffer[i] < 16) Serial.print("0");
        Serial.print(_responseBuffer[i], HEX);
        Serial.print(" ");
        if (i > 1 && i % 32 == 31) {
            Serial.println(" ");
        }
    }
    Serial.println();
}
