#include "ha_helper.h"
#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include <ArduinoJson.h>
#include "component_includer.h"

// Core objects
WiFiClient wifiClient;
HADevice device;
HAMqtt mqtt(wifiClient, device);

// Entities
// AC Control
mkswitch(ac_control_power);
mknumber(ac_control_temp);
mknumber(ac_control_fan);

// Fan Control
mkswitch(fan_reverse);
mkswitch(fan_light);
mknumber(fan_speed);
mknumber(fan_timer);

// Climate Control
mkswitch(climate_enable)
mknumber(climate_temp)
mkswitch(climate_boost)
mkswitch(climate_sleep);

mkswitch(is_available);



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


HAMqtt* getMqtt() {
    return &mqtt;
}

HADevice* getDevice() {
    return &device;
}

void handleUpdate(StaticJsonDocument<256>& doc){
    /*
    * Handle the update message from Home Assistant
    * This function is called when a message is received on the MQTT_UPDATE_TOPIC
    * 
    * The format of the message should be as follows:
    * {
    *     "ac_control_power": boolean,
    *     "ac_control_temp": int16,
    *     "ac_control_fan": int16,
    *     "fan_reverse": boolean,
    *     "fan_light": boolean,
    *     "fan_speed": int16,
    *     "fan_timer": unsigned long,
    *     "climate_enable": boolean,
    *     "climate_temp": int16
    * }
    * 
    * any one of the above keys can be present or absent in the message, and only the present keys will be updated.
    */
    #if AC_CONTROL != 0
    ac_set_state(doc);
    #endif
    #if CLIMATE_CONTROL != 0
    climate_set_state(doc);
    #endif
    #if FAN != 0
    fan_set_state(doc);
    #endif
    #if ALARM != 0
    alarm_set_state(doc);
    #endif
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
    }else if(strcmp(topic, MQTT_UPDATE_TOPIC) == 0) {
        Serial.print("Update received: ");
        char buffer[length + 1];
        memcpy(buffer, payload, length);
        buffer[length] = '\0';
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, buffer);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        handleUpdate(doc);
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

    climate_enable.setName("Climate Enable");
    
    climate_temp.setName("Climate Temp");
    climate_temp.setUnitOfMeasurement("°C");
    climate_temp.setDeviceClass("temperature");

    is_available.setName("Is Available");

    climate_boost.setName("Climate Boost");
    climate_sleep.setName("Climate Sleep");

    ac_control_power.setRetain(true);
    fan_reverse.setRetain(true);
    fan_light.setRetain(true);
    climate_enable.setRetain(true);


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
        mqtt.subscribe(MQTT_UPDATE_TOPIC);
        mqtt.publish("time/request", "epoch");
        is_available.setState(true);
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
