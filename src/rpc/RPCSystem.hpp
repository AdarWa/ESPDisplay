#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <functional>
#include <map>

class ESP32RPC {
public:
    using Callback = std::function<JsonVariant(JsonVariantConst)>;

    ESP32RPC(PubSubClient &client, const String &uuid_file = "/uuid.txt");

    bool begin();
    void loop();

    int getUUID() const { return uuid; }

    JsonDocument call(const String &method, JsonVariantConst params, unsigned long timeout = 5000);
    void registerMethod(const String &name, Callback cb);

private:
    PubSubClient &mqtt;
    int uuid = -1;
    String uuid_file;
    std::map<String, Callback> methods;

    struct Pending {
      bool done = false;
      JsonDocument doc; // holds either result or error form
      Pending(): doc() {}
    };
    std::map<String, Pending*> pending;

    // topics
    String topicServer() const; // server publishes requests here, device must subscribe
    String topicClient() const; // server listens here, device publishes requests and responses

    // handshake
    bool requestUUID();
    bool loadUUID();
    bool saveUUID();

    // message handling
    static void mqttThunk(char* topic, byte* payload, unsigned int length);
    void onMQTT(char* topic, byte* payload, unsigned int length);

    // JSON-RPC helpers
    String newId();
    void sendJSON(const String &topic, JsonDocument const &doc);
    void handleIncomingJSON(const String &topic, JsonDocument const &doc);
    void handleIncomingRequest(JsonDocument const &doc);
    void handleIncomingResponse(JsonDocument const &doc);
};

class RPCSystem {
public:
    RPCSystem(
        const char* ssid,
        const char* password,
        const char* mqtt_server,
        int mqtt_port = 1883,
        const char* mqtt_user = nullptr,
        const char* mqtt_pass = nullptr
    );

    bool begin();
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

    bool initSPIFFS();
    bool connectWiFi();
    bool connectMQTT();
};
