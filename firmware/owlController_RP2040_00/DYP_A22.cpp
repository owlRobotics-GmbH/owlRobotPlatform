#include "DYP_A22.h"
#include "Funkt.h"
#include <TCA9548A.h>

extern TCA9548A I2CMux;

DYP_A22::DYP_A22(uint8_t mux_channel, uint8_t i2c_address)
    : _mux_channel(mux_channel), _i2c_address(i2c_address), _isConnected(false) {}

bool DYP_A22::begin() {
    resetMeasurement();
    const uint8_t maxAttempts = 5;
    for (uint8_t attempt = 0; attempt < maxAttempts; ++attempt) {
        if (probeSensor()) {
            return true;
        }
        delay(30);
    }
    return false;
}

bool DYP_A22::triggerMeasurement(uint8_t level) {
    static const uint8_t cmd[] = { 0xBD, 0xBC, 0xBB, 0xB4 };
    if (level < 1 || level > 4) level = 1;
    if (!ensureConnected()) return false;
    I2CMux.openChannel(_mux_channel);
    delayMicroseconds(150);
    
    Wire.beginTransmission(_i2c_address);
    Wire.write(0x10);     // Control register
    Wire.write(cmd[level - 1]);     // Trigger range level
    int result = Wire.endTransmission();
    if (result != 0) {
        _isConnected = false;
        resetMeasurement();
        return false;
    }
    return true;
}

uint16_t DYP_A22::readRegister16(uint8_t reg) {
    if (!ensureConnected()) return 0xFFFF;
    I2CMux.openChannel(_mux_channel);
    delayMicroseconds(150);

    Wire.beginTransmission(_i2c_address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        _isConnected = false;
        return 0xFFFF;
    }

    Wire.requestFrom(_i2c_address, (uint8_t)2);
    if (Wire.available() < 2) {
        return 0xFFFF;
    }

    uint8_t high = Wire.read();
    uint8_t low  = Wire.read();

    return (high << 8) | low;
}

bool DYP_A22::writeRegister8(uint8_t reg, uint8_t value) {
    if (!ensureConnected()) return false;
    I2CMux.openChannel(_mux_channel);
    delayMicroseconds(150);

    Wire.beginTransmission(_i2c_address);
    Wire.write(reg);
    Wire.write(value);
    int result = Wire.endTransmission();
    if (result != 0) {
        _isConnected = false;
        resetMeasurement();
        return false;
    }
    return true;
}

void DYP_A22::resetMeasurement() {
    _measurementInProgress = false;
    _measurementStartMs = 0;
}

bool DYP_A22::startMeasurement(uint8_t angleLevel, uint8_t rangeLevel) {
    if (!ensureConnected()) {
        resetMeasurement();
        return false;
    }
    if (angleLevel < 1 || angleLevel > 4) angleLevel = 4;
    if (rangeLevel < 1 || rangeLevel > 4) rangeLevel = 1;
    if (!writeRegister8(0x07, angleLevel)) {
        if (_isConnected) {
            Serial.println("Fehler: Winkellevel setzen");
        }
        resetMeasurement();
        return false;
    }
    if (!triggerMeasurement(rangeLevel)) {
        if (_isConnected) {
            Serial.println("Fehler: Messung ausloesen");
        }
        resetMeasurement();
        return false;
    }
    _measurementStartMs = millis();
    _measurementInProgress = true;
    return true;
}

bool DYP_A22::pollDistance(uint8_t angleLevel, uint16_t &distance, uint8_t rangeLevel) {
    if (!ensureConnected()) {
        resetMeasurement();
        return false;
    }

    unsigned long now = millis();

    if (!_measurementInProgress) {
        startMeasurement(angleLevel, rangeLevel);
        return false;
    }

    if ((unsigned long)(now - _measurementStartMs) < kMeasurementDelayMs) {
        return false;
    }

    uint16_t value = readRegister16(RO_Distance_value);
    _measurementInProgress = false;

    if (value == 0xFFFF) {
        startMeasurement(angleLevel, rangeLevel);
        return false;
    }

    distance = value;
    startMeasurement(angleLevel, rangeLevel);
    return true;
}

bool DYP_A22::ensureConnected() {
    if (_isConnected) return true;
    unsigned long now = millis();
    if (now - _lastProbeMs < 50) {
        return false;
    }
    _lastProbeMs = now;
    if (probeSensor()) {
        return true;
    }
    resetMeasurement();
    return false;
}

bool DYP_A22::probeSensor() {
    I2CMux.openChannel(_mux_channel);
    delay(5); // give the mux and sensor a moment to settle

    int result = 0;
    _lastProbeMs = millis();

    Wire.beginTransmission(_i2c_address);
    Wire.write(RO_Software_version);
    result = Wire.endTransmission();

    if (result != 0) {
        Wire.beginTransmission(_i2c_address);
        result = Wire.endTransmission();
    }

    if (result != 0) {
        _isConnected = false;
        return false;
    }

    int received = Wire.requestFrom(_i2c_address, (uint8_t)2);
    if (received >= 1) {
        while (Wire.available()) {
            (void)Wire.read();
        }
    }

    _isConnected = true;
    return true;
}

uint16_t DYP_A22::getDistance(uint8_t angleLevel) {
    resetMeasurement();
    if (!startMeasurement(angleLevel, 1)) {
        return 0xFFFF;
    }

    while ((unsigned long)(millis() - _measurementStartMs) < kMeasurementDelayMs) {
        delay(5);
    }

    uint16_t value = readRegister16(RO_Distance_value);
    _measurementInProgress = false;
    return value;
}

void DYP_A22::printInfo() {
    if (!ensureConnected()) {
        return;
    }
    if(_i2c_address == 0x74) {
        Serial.print("Sensor: L");
        //Serial.println();
        //Serial.print(F("DYP-A22 Infos:   "));
        //Serial.print(F("  MUX-Kanal: ")); Serial.print(_mux_channel);
        //Serial.print(F("  I2C-Adresse: 0x")); Serial.print(_i2c_address, HEX);

        // Softwareversion
        uint16_t version = readRegister16(RO_Software_version);
        if (version != 0xFFFF) {
            //Serial.print(F("  Softwareversion: 0x")); Serial.print(version, HEX);
        } else {
            //Serial.print(F("  Softwareversion: Fehler beim Lesen"));
        }

        // Temperatur
        int16_t temp = (int16_t)readRegister16(RO_Temperature_value);
        if (temp != 0xFFFF) {
            Serial.print(F("  Temperatur: "));
            Serial.print(temp / 10.0, 1);
            Serial.print(F(" °C"));
        } else {
            Serial.print(F("  Temperatur:         "));
        }

        // Testdistanz
        dCounter++;
        uint16_t dist = getDistance(dCounter % 4 + 1);
        if (dist < 0xF000) {
            if (dCounter % 4 == 0) {
                Serial.print(F("  Winkel: 60°"));
            } else if (dCounter % 4 == 1) {
                Serial.print(F("  Winkel: 30°"));
            } else if (dCounter % 4 == 2) {
                Serial.print(F("  Winkel: 40°"));
            } else {
                Serial.print(F("  Winkel: 50°"));
            }
            Serial.print(F("  Testdistanz: "));
            Serial.print(dist);
            Serial.println(F(" mm"));
        } else {
            if (dCounter % 4 == 0) {
                Serial.print(F("  Winkel: 60°"));
            } else if (dCounter % 4 == 1) {
                Serial.print(F("  Winkel: 30°"));
            } else if (dCounter % 4 == 2) {
                Serial.print(F("  Winkel: 40°"));
            } else {
                Serial.print(F("  Winkel: 50°"));
            }
            Serial.println(F("  Testdistanz:"));
        }
    } else if(_i2c_address == 0x75) {
        Serial.print("Sensor: R");
        //Serial.println();
        //Serial.print(F("DYP-A22 Infos:   "));
        //Serial.print(F("  MUX-Kanal: ")); Serial.print(_mux_channel);
        //Serial.print(F("  I2C-Adresse: 0x")); Serial.print(_i2c_address, HEX);

        // Softwareversion
        uint16_t version = readRegister16(RO_Software_version);
        if (version != 0xFFFF) {
            //Serial.print(F("  Softwareversion: 0x")); Serial.print(version, HEX);
        } else {
            //Serial.print(F("  Softwareversion: Fehler beim Lesen"));
        }

        // Temperatur
        int16_t temp = (int16_t)readRegister16(RO_Temperature_value);
        if (temp != 0xFFFF) {
            Serial.print(F("  Temperatur: "));
            Serial.print(temp / 10.0, 1);
            Serial.print(F(" °C"));
        } else {
            Serial.print(F("  Temperatur:         "));
        }

        // Testdistanz
        dCounter++;
        uint16_t dist = getDistance(dCounter % 4 + 1);
        if (dist < 0xF000) {
            if (dCounter % 4 == 0) {
                Serial.print(F("  Winkel: 60°"));
            } else if (dCounter % 4 == 1) {
                Serial.print(F("  Winkel: 30°"));
            } else if (dCounter % 4 == 2) {
                Serial.print(F("  Winkel: 40°"));
            } else {
                Serial.print(F("  Winkel: 50°"));
            }
            Serial.print(F("  Testdistanz: "));
            Serial.print(dist);
            Serial.println(F(" mm"));
        } else {
            if (dCounter % 4 == 0) {
                Serial.print(F("  Winkel: 60°"));
            } else if (dCounter % 4 == 1) {
                Serial.print(F("  Winkel: 30°"));
            } else if (dCounter % 4 == 2) {
                Serial.print(F("  Winkel: 40°"));
            } else {
                Serial.print(F("  Winkel: 50°"));
            }
            Serial.println(F("  Testdistanz:"));
        }
    } else {
        Serial.print(F("DYP_A22 Sensor Unbekannt:"));
    }
    
}
