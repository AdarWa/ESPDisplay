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


void ha_begin();
void ha_loop();

#endif
