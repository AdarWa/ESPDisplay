#include <vector>
#include "base/Screen.h"
#include "spiffs_handler.h"

#define CONFIG_BUFFER_SIZE 16384 // I fucking hate buffers. Change this stupid shit if it causes problems.


class Config {
public:
    std::vector<std::shared_ptr<Screen>> screens;

    static Config parseConfig(const ArduinoJson::V742PB22::JsonVariant& config) {
        Config cfg;

        if (!config.is<JsonObject>()) return cfg;

        auto obj = config.as<JsonObject>();
        if (!obj.containsKey("screens") || !obj["screens"].is<JsonArray>()) return cfg;

        auto screensArr = obj["screens"].as<JsonArray>();
        for (auto s : screensArr) {
            if (!s.is<JsonObject>()) continue;
            auto scrObj = s.as<JsonObject>();

            std::string name = scrObj["name"] | "unnamed";
            std::string backScreen = scrObj["backScreen"] | scrObj["back_screen"] | "";

            auto screen = std::make_shared<Screen>(name, backScreen);

            // Components
            if (scrObj.containsKey("components") && scrObj["components"].is<JsonArray>()) {
                auto compsArr = scrObj["components"].as<JsonArray>();
                for (auto c : compsArr) {
                    if (!c.is<JsonObject>()) continue;
                    auto compObj = c.as<JsonObject>();

                    std::string uuid = compObj["uuid"] | compObj["comp_id"] | "";
                    auto comp = std::make_shared<Component>(uuid);

                    if (compObj.containsKey("type") && compObj["type"].is<const char*>()) {
                        comp->type = compObj["type"].as<const char*>();
                    }
                    if (compObj.containsKey("params") && compObj["params"].is<JsonObject>()) {
                        auto paramsObj = compObj["params"].as<JsonObject>();
                        for (auto kv : paramsObj) {
                            if (kv.value().is<const char*>())
                                comp->params[kv.key().c_str()] = kv.value().as<const char*>();
                        }
                    }

                    screen->addComponent(comp);
                }
            }

            cfg.screens.push_back(screen);
        }

        return cfg;
    }
};



class ConfigManager {
private:
    ConfigManager() = default;

    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::unique_ptr<Config> config_;

public:
    ~ConfigManager() = default;

    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool load(const char* path = "/config.json") {
        DynamicJsonDocument doc(CONFIG_BUFFER_SIZE);  // adjust capacity as needed
        if (!spiffs_file_exists(path)) {
            return false; // no config file yet
        }
        if (!spiffs_load_json(path, doc)) {
            return false; // failed to load
        }

        JsonVariant root = doc.as<JsonVariant>();
        config_ = std::make_unique<Config>(Config::parseConfig(root));
        return true;
    }

    bool save(const char* path = "/config.json") {
        if (!config_) return false;

        DynamicJsonDocument doc(CONFIG_BUFFER_SIZE);
        JsonObject root = doc.to<JsonObject>();

        // Serialize current config back to JSON
        JsonArray screensArr = root.createNestedArray("screens");
        for (auto& scr : config_->screens) {
            JsonObject scrObj = screensArr.createNestedObject();
            scrObj["name"] = scr->getName();
            if (!scr->getBackScreen().empty()) {
                scrObj["backScreen"] = scr->getBackScreen();
            }

            // Components
            JsonArray compsArr = scrObj.createNestedArray("components");
            for (auto& comp : scr->getComponents()) {
                JsonObject compObj = compsArr.createNestedObject();
                compObj["uuid"] = comp->getUUID();
                // you may want to expose getters for type and params
                // compObj["type"] = comp->getType();
                // JsonObject paramsObj = compObj.createNestedObject("params");
                // for (auto& kv : comp->getParams()) {
                //     paramsObj[kv.first.c_str()] = kv.second;
                // }
            }

            // Modules (similar idea)
            JsonArray modsArr = scrObj.createNestedArray("modules");
            for (auto& mod : scr->getModules()) {
                JsonObject modObj = modsArr.createNestedObject();
                // modObj["name"] = mod->getName();
            }
        }

        return spiffs_save_json(path, doc);
    }

    Config* getConfig() {
        return config_.get();
    }

    void setConfig(std::unique_ptr<Config> newConfig) {
        config_ = std::move(newConfig);
    }
};