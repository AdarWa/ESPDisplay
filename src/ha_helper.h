#ifndef HA_HELPER_H
#define HA_HELPER_H

#include <WiFi.h>
#include <ArduinoHA.h>

extern WiFiClient wifiClient;
extern HADevice device;
extern HAMqtt mqtt;

// AC Control
extern HASwitch ac_control_power;
extern HASensorNumber ac_control_temp;
extern HASensorNumber ac_control_fan;

// Fan Control
extern HASwitch fan_reverse;
extern HASwitch fan_light;
extern HASensorNumber fan_speed;
extern HASensorNumber fan_timer;

// Climate Control
extern HASwitch climate_enable;
extern HASensorNumber climate_temp;

extern HASwitch is_available;


// General Getters
HAMqtt* getMqtt();
HADevice* getDevice();


void ha_begin();
void ha_loop();
unsigned long getZeroEpochTime();
unsigned long getCurrentEpochTime();
unsigned long getRealEpochTime();

#endif
