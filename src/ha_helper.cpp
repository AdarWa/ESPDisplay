#include "ha_helper.h"
#include <WiFi.h>
#include "config.h"
#include "secrets.h"

// Core objects
WiFiClient wifiClient;
HADevice device;
HAMqtt mqtt(wifiClient, device);

// Entities
HASwitch ac_control_power("ac_control_power");
HASensorNumber ac_control_temp("ac_control_temp", HASensorNumber::PrecisionP0);
HASensorNumber ac_control_fan("ac_control_fan", HASensorNumber::PrecisionP0);

HASwitch fan_reverse("fan_reverse");
HASwitch fan_light("fan_light");
HASensorNumber fan_speed("fan_speed", HASensorNumber::PrecisionP0);

// Command handler for switch
static void onACSwitchCommand(bool state, HASwitch* sender) {
    Serial.printf("AC Power: %s\n", state ? "ON" : "OFF");
    sender->setState(state);
}

void ha_begin() {
    Serial.println("Setting up Home Assistant integration...");

    byte mac[6];
    WiFi.macAddress(mac);
    device.setUniqueId(mac, sizeof(mac));

    device.setName(MQTT_DEVICE_NAME);
    device.setSoftwareVersion("1.0.0");
    device.setManufacturer(MQTT_MANUFACTURER);
    device.setModel(MQTT_MODEL);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // AC power switch
    ac_control_power.setName("AC Control Power");
    ac_control_power.onCommand(onACSwitchCommand);

    fan_reverse.setName("Fan Reverse");
    fan_reverse.onCommand(onACSwitchCommand);

    fan_light.setName("Fan Light");
    fan_light.onCommand(onACSwitchCommand);

    // Temperature sensor
    ac_control_temp.setName("AC Control Temp");
    ac_control_temp.setUnitOfMeasurement("°C");
    ac_control_temp.setDeviceClass("temperature");

    // Fan level sensor
    ac_control_fan.setName("AC Control Fan");
    fan_speed.setName("Fan Speed");

    // Register entities
    mqtt.addDeviceType(&ac_control_power);
    mqtt.addDeviceType(&ac_control_temp);
    mqtt.addDeviceType(&ac_control_fan);
    mqtt.addDeviceType(&fan_light);
    mqtt.addDeviceType(&fan_reverse);
    mqtt.addDeviceType(&fan_speed);

    if(mqtt.begin(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD)){
        Serial.println("MQTT connected");
    } else {
        Serial.println("MQTT connection failed");
    }
}

void ha_loop() {
    mqtt.loop();
}
