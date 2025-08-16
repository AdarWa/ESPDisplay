#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <functional>
#include <map>
#include <string>
#include <FS.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include "../spiffs_handler.h"

class ESP32RPC {
public:
    using Callback = std::function<JsonVariant(JsonVariant)>;

    ESP32RPC(PubSubClient &client, const std::string &uuid_file = "/uuid.txt");

    // initialize SPIFFS, MQTT, and subscribe handshake
    bool begin();

    int getUUID() const;

    // call server method
    JsonVariant call(const std::string &method, JsonVariant params, unsigned long timeout = 5000);

    void registerMethod(const std::string &name, Callback cb);

    void loop();

private:
    PubSubClient &mqtt;
    int uuid;
    std::string uuid_file;
    std::map<String, Callback> methods;
    std::map<String, JsonVariant> responses;

    String serverTopic();
    String clientTopic();

    bool requestUUID();
    void subscribeTopics();
    void onMessage(char* topic, byte* payload, unsigned int length);

    String temp_request_id;
};
