#include "DYP_A22.h"
#include "Funkt.h"
#include <TCA9548A.h>

extern TCA9548A I2CMux;

DYP_A22::DYP_A22(uint8_t mux_channel, uint8_t i2c_address) : _mux_channel(mux_channel), _i2c_address(i2c_address) {}

void DYP_A22::begin() {
    I2CMux.openChannel(_mux_channel);
}

bool DYP_A22::triggerMeasurement(uint8_t level) {
    static const uint8_t cmd[] = { 0xBD, 0xBC, 0xBB, 0xB4 };
    if (level < 1 || level > 4) level = 1;
    I2CMux.openChannel(_mux_channel);

    delayMicroseconds(10);
    
    Wire.beginTransmission(_i2c_address);
    Wire.write(0x10);     // Control register
    Wire.write(cmd[level - 1]);     // Trigger range level
    return Wire.endTransmission() == 0;
}

uint16_t DYP_A22::readRegister16(uint8_t reg) {
    I2CMux.openChannel(_mux_channel);

    Wire.beginTransmission(_i2c_address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return 0xFFFF;

    Wire.requestFrom(_i2c_address, (uint8_t)2);
    if (Wire.available() < 2) return 0xFFFF;

    uint8_t high = Wire.read();
    uint8_t low  = Wire.read();

    return (high << 8) | low;
}

bool DYP_A22::writeRegister8(uint8_t reg, uint8_t value) {
    I2CMux.openChannel(_mux_channel);

    Wire.beginTransmission(_i2c_address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

uint16_t DYP_A22::getDistance(uint8_t angleLevel) {
    // 1) Winkellevel setzen (Register 0x07, Werte 1..4)
    if (angleLevel < 1 || angleLevel > 4) angleLevel = 4;
    if (!writeRegister8(0x07, angleLevel)) {
        Serial.println("Fehler: Winkellevel setzen");
        return 0xFFFF;
    }

    // 2) Messung auslösen
    if (!triggerMeasurement(1)) {
        Serial.println("Fehler: Messung ausloesen");
        return 0xFFFF;
    }

    // 3) Wartezeit lt. Datenblatt 50–80ms
    delay(90);

    // 4) Distanz auslesen
    return readRegister16(RO_Distance_value);
}

void DYP_A22::printInfo() {
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
