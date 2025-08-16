#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include "ESP32RPC.hpp"

class RPCSystem {
public:
    RPCSystem(
        const char* ssid,
        const char* password,
        const char* mqtt_server,
        int mqtt_port = 1883,
        const char* mqtt_user = nullptr,
        const char* mqtt_pass = nullptr
    )
        : ssid(ssid), password(password),
          mqtt_server(mqtt_server), mqtt_port(mqtt_port),
          mqtt_user(mqtt_user), mqtt_pass(mqtt_pass),
          mqtt(client), rpc(mqtt) {}

    bool begin() {
        if (!initSPIFFS()) return false;
        if (!connectWiFi()) return false;
        if (!connectMQTT()) return false;
        if (!rpc.begin()) return false;
        return true;
    }

    ESP32RPC& getRPC() { return rpc; }
    PubSubClient& getMQTT() { return mqtt; }

private:
    const char* ssid;
    const char* password;
    const char* mqtt_server;
    int mqtt_port;
    const char* mqtt_user;
    const char* mqtt_pass;

    WiFiClient client;
    PubSubClient mqtt;
    ESP32RPC rpc;

    bool initSPIFFS() {
        if (!SPIFFS.begin(true)) {
            Serial.println("Failed to mount SPIFFS");
            return false;
        }
        return true;
    }

    bool connectWiFi() {
        Serial.print("Connecting to WiFi");
        WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            if (millis() - start > 20000) { // 20 sec timeout
                Serial.println("WiFi connection failed");
                return false;
            }
        }
        Serial.println();
        Serial.print("Connected to WiFi, IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    bool connectMQTT() {
        mqtt.setServer(mqtt_server, mqtt_port);
        Serial.print("Connecting to MQTT");
        unsigned long start = millis();
        while (!mqtt.connected()) {
            bool ok;
            if (mqtt_user && mqtt_pass) {
                ok = mqtt.connect("esp32client", mqtt_user, mqtt_pass);
            } else {
                ok = mqtt.connect("esp32client");
            }
            if (ok) {
                Serial.println("MQTT connected");
                break;
            }
            delay(500);
            Serial.print(".");
            if (millis() - start > 10000) { // 10 sec timeout
                Serial.println("MQTT connection failed");
                return false;
            }
        }
        return true;
    }
};
