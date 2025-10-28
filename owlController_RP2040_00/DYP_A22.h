#ifndef DYP_A22_H
#define DYP_A22_H

#include <Arduino.h>
#include <Wire.h>
#include "Funkt.h"

#define DYP1_A22_I2C_ADDRESS 0x74  // 7-Bit I2C-Adresse (entspricht 0xE8)
#define DYP2_A22_I2C_ADDRESS 0x75  // 7-Bit I2C-Adresse (entspricht 0xE8)
// Read-only Register
#define RO_Software_version 0x00   // Register für Softwareversion
#define RO_Temperature_value 0x0A  // Register für Temperaturmessung
#define RO_Distance_value 0x02 // Register für Distanzmessung

class DYP_A22 {
public:
    DYP_A22(uint8_t mux_channel, uint8_t i2c_address);
    bool begin();
    uint16_t getDistance(uint8_t angleLevel = 4);    // Abstand in mm
    bool isConnected() const { return _isConnected; }
    bool pollDistance(uint8_t angleLevel, uint16_t &distance, uint8_t rangeLevel = 1);
    void resetMeasurement();
    
    void printInfo();          // Gibt Infos aus

private:
    uint8_t _mux_channel;
    uint8_t _i2c_address;
    int dCounter = 0;   // Zähler für Distanzmessungen
    bool _isConnected = false;
    bool _measurementInProgress = false;
    unsigned long _measurementStartMs = 0;
    unsigned long _lastProbeMs = 0;
    static constexpr uint16_t kMeasurementDelayMs = 90;
    bool triggerMeasurement(uint8_t level = 1);
    // in mm: 0xBD für Range-Level-1 (50cm), 0xBC für Range-Level-2(150cm), 0xB8 für Range-Level-3 (250cm), 0xB4 für Range-Level-4 (350cm)
    // in us: 0x05 für Range-Level-1 (50cm), 0x0A für Range-Level-2(150cm), 0x0F für Range-Level-3 (250cm), 0xB2 für Range-Level-4 (350cm)
    bool ensureConnected();
    bool probeSensor();
    uint16_t readRegister16(uint8_t reg);
    bool writeRegister8(uint8_t reg, uint8_t value);
    bool startMeasurement(uint8_t angleLevel, uint8_t rangeLevel);
};
#endif
