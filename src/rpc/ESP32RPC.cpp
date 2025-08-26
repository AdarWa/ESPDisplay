#include "ESP32RPC.hpp"

ESP32RPC::ESP32RPC(PubSubClient &client, const std::string &uuid_file)
    : mqtt(client), uuid_file(uuid_file), uuid(-1) {}  // initialize uuid as invalid

bool ESP32RPC::begin() {
    LV_LOG_USER("[ESP32RPC] SPIFFS initialized");

    if (SPIFFS.exists(uuid_file.c_str())) {
        File f = SPIFFS.open(uuid_file.c_str(), "r");
        if (f) {
            uuid = f.readString().toInt();
            f.close();
            LV_LOG_USER("[ESP32RPC] Loaded UUID from SPIFFS: %d", uuid);
        }
    }

    mqtt.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->onMessage(topic, payload, length);
    });

    if (uuid < 0) {  // no saved UUID
        LV_LOG_USER("[ESP32RPC] No saved UUID, requesting from server");
        mqtt.subscribe("espdisplay/broadcast");
        if(!requestUUID()){
            LV_LOG_USER("[ESP32RPC] Failed to request UUID from server.");
            return false;
        }
    }

    subscribeTopics();
    LV_LOG_USER("[ESP32RPC] Subscribed to topics for UUID %d", uuid);
    return true;
}

int ESP32RPC::getUUID() const {
    return uuid;
}

JsonVariant ESP32RPC::call(const std::string &method, JsonVariant params, unsigned long timeout) {
    if (uuid < 0) return JsonVariant(); // not initialized

    String id = String(random(1, 0xFFFF), HEX);
    DynamicJsonDocument doc(512);
    doc["jsonrpc"] = "2.0";
    doc["method"] = method.c_str();
    doc["params"] = params;
    doc["id"] = id;

    String msg;
    serializeJson(doc, msg);
    LV_LOG_USER("[ESP32RPC] published data=%s", msg.c_str());
    mqtt.publish(clientTopic().c_str(), msg.c_str());

    unsigned long start = millis();
    while (millis() - start < timeout) {
        mqtt.loop();
        if (responses.count(id) > 0) {
            JsonVariant result = responses[id];
            responses.erase(id);
            return result;
        }
        delay(1);
    }
    return JsonVariant();
}

void ESP32RPC::registerMethod(const std::string &name, Callback cb) {
    methods[String(name.c_str())] = cb;
}

void ESP32RPC::loop() {
    mqtt.loop();
}

String ESP32RPC::serverTopic() {
    return "espdisplay/" + String(uuid) + "/server";
}

String ESP32RPC::clientTopic() {
    return "espdisplay/" + String(uuid) + "/client";
}

bool ESP32RPC::requestUUID() {
    temp_request_id = String(random(1, 0xFFFF), HEX);

    DynamicJsonDocument doc(256);
    doc["request_id"] = temp_request_id;
    doc["request_type"] = "subscribe";

    String msg;
    serializeJson(doc, msg);
    mqtt.publish("espdisplay/subscribe", msg.c_str());

    unsigned long start = millis();
    while (millis() - start < 5000) {
        mqtt.loop();
        if (uuid >= 0) return true;
        delay(1);
    }
    return false;
}

void ESP32RPC::subscribeTopics() {
    mqtt.subscribe(serverTopic().c_str());
}

void ESP32RPC::onMessage(char* topic, byte* payload, unsigned int length) {
    String t = String(topic);
    LV_LOG_USER("[ESP32RPC] MQTT message received on topic: %s", t.c_str());

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        LV_LOG_ERROR("[ESP32RPC] Failed to deserialize JSON: %s", err.c_str());
        return;
    }

    // handle subscribe reply
    if (t == "espdisplay/broadcast") {
        String rid = doc["request_id"] | "";
        String type = doc["request_type"] | "";
        LV_LOG_USER("[ESP32RPC] Received broadcast: request_id=%s, type=%s", rid.c_str(), type.c_str());

        if (rid == temp_request_id && type == "subscribe_reply") {
            int new_uuid = doc["uuid"] | -1;
            if (new_uuid >= 0) {
                uuid = new_uuid;
                File f = SPIFFS.open(uuid_file.c_str(), "w");
                if (f) {
                    f.print(String(uuid));
                    f.close();
                    LV_LOG_USER("[ESP32RPC] UUID saved to SPIFFS: %d", uuid);
                }
                subscribeTopics();
                LV_LOG_USER("[ESP32RPC] Subscribed to client topic after UUID assignment");
            }
            return;
        }
    }

    // handle RPC requests
    if (t == serverTopic()) {
        if (doc.containsKey("method")) {
            String method = doc["method"].as<String>();
            String id = doc["id"].as<String>();
            LV_LOG_USER("[ESP32RPC] Received RPC request: method=%s, id=%s", method.c_str(), id.c_str());

            DynamicJsonDocument resp(512);
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;

            if (methods.count(method) > 0) {
                try {
                    JsonVariant result = methods[method](doc["params"]);
                    resp["result"] = result;
                    LV_LOG_USER("[ESP32RPC] RPC method %s executed successfully", method.c_str());
                } catch (...) {
                    resp["error"]["code"] = -32603;
                    resp["error"]["message"] = "Internal error";
                    LV_LOG_ERROR("[ESP32RPC] RPC method %s threw exception", method.c_str());
                }
            } else {
                resp["error"]["code"] = -32601;
                resp["error"]["message"] = "Method not found";
                LV_LOG_WARN("[ESP32RPC] RPC method %s not found", method.c_str());
            }

            String msg;
            serializeJson(resp, msg);
            mqtt.publish(clientTopic().c_str(), msg.c_str());
            LV_LOG_USER("[ESP32RPC] Published RPC response to %s", clientTopic().c_str());
        } else if (doc.containsKey("result") || doc.containsKey("error")) {
            String id = doc["id"].as<String>();
            responses[id] = doc.containsKey("result") ? doc["result"] : doc["error"];
            LV_LOG_USER("[ESP32RPC] Stored RPC response for id=%s", id.c_str());
        }
    }
}
