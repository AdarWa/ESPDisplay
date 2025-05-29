#ifndef HA_HELPER_H
#define HA_HELPER_H

#include <WiFi.h>
#include <ArduinoHA.h>

// Macros for defining switches and numbers sensors for Home Assistant
#define mkextswitch(name) extern HASwitch name;
#define mkextnumber(name) extern HASensorNumber name;

#define mkswitch(name) HASwitch name(#name);
#define mknumber(name) HASensorNumber name(#name,HASensorNumber::PrecisionP0);


extern WiFiClient wifiClient;
extern HADevice device;
extern HAMqtt mqtt;

// AC Control
mkextswitch(ac_control_power);
mkextnumber(ac_control_temp);
mkextnumber(ac_control_fan);

// Fan Control
mkextswitch(fan_reverse);
mkextswitch(fan_light);
mkextnumber(fan_speed);
mkextnumber(fan_timer);

// Climate Control
mkextswitch(climate_enable)
mkextnumber(climate_temp)
mkextswitch(climate_boost)
mkextswitch(climate_sleep)

mkextswitch(is_available);


// General Getters
HAMqtt* getMqtt();
HADevice* getDevice();


void ha_begin();
void ha_loop();
unsigned long getZeroEpochTime();
unsigned long getCurrentEpochTime();
unsigned long getRealEpochTime();

#endif
