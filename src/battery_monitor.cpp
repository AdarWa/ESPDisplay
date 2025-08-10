#include "battery_monitor.h"
#include <Wire.h>
#include "config.h"
#if ENABLE_BATTERY != 0
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

SFE_MAX1704X gauge;
#else
#include "Arduino.h"
#endif

float soc;
float voltage;

bool battery_init() {
    #if ENABLE_BATTERY != 0
    Wire.begin();
    gauge.enableDebugging();

    if (gauge.begin() == false){
        Serial.println(F("MAX17043 not detected. Please check wiring."));
        return false;
    }
	gauge.quickStart();
    return true;
    #else
    Serial.println(F("Battery monitoring is disabled."));
    return false;
    #endif
}

void battery_update() {
    #if ENABLE_BATTERY != 0
    soc = gauge.getSOC();
    voltage = gauge.getVoltage();
    #endif
}

// Returns the battery's SOC in floating point precentage (0-100).
float battery_get_level() {
  return soc;
}

// Returns the battery's voltage in floating point volts (0-4.2).
float battery_get_voltage() {
  return voltage;
}
