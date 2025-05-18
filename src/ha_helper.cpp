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
HASensorNumber fan_timer("fan_timer", HASensorNumber::PrecisionP0);

static unsigned long zero_epoch = 0;

// Command handler for switch
static void onACSwitchCommand(bool state, HASwitch* sender) {
    Serial.printf("AC Power: %s\n", state ? "ON" : "OFF");
    sender->setState(state);
}

unsigned long getZeroEpochTime() {
    return zero_epoch;
}

unsigned long getCurrentEpochTime() {
    return zero_epoch + (millis() / 1000);
}

unsigned long getRealEpochTime() {
    return zero_epoch + (millis() / 1000) + EPOCH_ZERO;
}

void onCustomMessage(const char* topic, const uint8_t* payload, uint16_t length) {
    Serial.print("Received message on topic: ");
    Serial.println(topic);


    if(strcmp(topic, "time/epoch") == 0) {
        char buffer[length + 1];
        Serial.print("Epoch time received: ");
        memcpy(buffer, payload, length);
        buffer[length] = '\0';

        unsigned long epoch = strtoul(buffer, NULL, 10);
        epoch = epoch-EPOCH_ZERO - (millis()/1000);
        Serial.print("Epoch received: ");
        Serial.println(epoch);
        zero_epoch = epoch;
    }
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
    fan_timer.setName("Fan Timer");

    ac_control_power.setRetain(true);
    fan_reverse.setRetain(true);
    fan_light.setRetain(true);


    // Register entities
    mqtt.addDeviceType(&ac_control_power);
    mqtt.addDeviceType(&ac_control_temp);
    mqtt.addDeviceType(&ac_control_fan);
    mqtt.addDeviceType(&fan_light);
    mqtt.addDeviceType(&fan_reverse);
    mqtt.addDeviceType(&fan_speed);
    mqtt.addDeviceType(&fan_timer);
    mqtt.onMessage(onCustomMessage);

    mqtt.onConnected([]() {
        Serial.println("MQTT connected!");
        mqtt.subscribe("time/epoch");
        mqtt.publish("time/request", "epoch");
    });
    if(mqtt.begin(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD)){
        Serial.println("MQTT connection initialized");
    } else {
        Serial.println("MQTT connection failed");
    }

}

void ha_loop() {
    mqtt.loop();
}
