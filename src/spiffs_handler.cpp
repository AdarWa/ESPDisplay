// spiffs_handler.cpp
#include "spiffs_handler.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

bool spiffs_begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[SPIFFS] Mount failed");
        return false;
    }
    Serial.println("[SPIFFS] Mounted successfully");
    return true;
}

bool spiffs_save_json(const char* path, const JsonDocument& doc) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("[SPIFFS] Failed to open %s for writing\n", path);
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.printf("[SPIFFS] Failed to write JSON to %s\n", path);
        file.close();
        return false;
    }

    file.close();
    Serial.printf("[SPIFFS] JSON saved to %s\n", path);
    return true;
}

bool spiffs_load_json(const char* path, JsonDocument& doc) {
    File file = SPIFFS.open(path);
    if (!file || file.isDirectory()) {
        Serial.printf("[SPIFFS] Failed to open %s for reading\n", path);
        return false;
    }

    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.printf("[SPIFFS] Failed to parse JSON from %s: %s\n", path, error.c_str());
        file.close();
        return false;
    }

    file.close();
    Serial.printf("[SPIFFS] JSON loaded from %s\n", path);
    return true;
}

bool spiffs_remove_file(const char* path) {
    if (!SPIFFS.exists(path)) {
        Serial.printf("[SPIFFS] File %s does not exist\n", path);
        return false;
    }

    return SPIFFS.remove(path);
}

bool spiffs_file_exists(const char* path) {
    return SPIFFS.exists(path);
} 