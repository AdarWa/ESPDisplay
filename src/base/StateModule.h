/*#pragma once
#include <ArduinoJson.h>
#include "spiffs_handler.h"
#include "PageBase.h"

class StateModule : public Module {
public:
    StateModule(const char* filepath)
        : _filepath(filepath) {}

    virtual ~StateModule() = default;

    // Load state from SPIFFS JSON file
    bool load() {
        if (!spiffs_file_exists(_filepath)) {
            save();  // Save defaults if not exists
            return true;
        }
        StaticJsonDocument<512> doc;
        if (spiffs_load_json(_filepath, doc)) {
            deserialize(doc);
            return true;
        }
        return false;
    }

    // Save current state to SPIFFS JSON file
    void save() {
        StaticJsonDocument<512> doc;
        serialize(doc);
        spiffs_save_json(_filepath, doc);
    }

    // To be implemented by subclasses: fill the JSON with current state
    virtual void serialize(StaticJsonDocument<512>& doc) = 0;

    // To be implemented by subclasses: load state from JSON
    virtual void deserialize(StaticJsonDocument<512>& doc) = 0;

    // Called after load/save to update MQTT or other listeners
    virtual void publish_update() = 0;

    // Load and publish combined method
    bool init() {
        bool ok = load();
        publish_update();
        return ok;
    }

protected:
    const char* _filepath;
};
*/