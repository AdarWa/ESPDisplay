#include <vector>
#include "base/Screen.h"
#include "spiffs_handler.h"

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
        DynamicJsonDocument doc(8192);  // adjust capacity as needed
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

        DynamicJsonDocument doc(8192);
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


class Config {
public:
    std::vector<std::shared_ptr<Screen>> screens;

    static Config parseConfig(JsonVariant& config) {
        Config cfg;

        if (!config.is<JsonObject>()) {
            // Invalid config, return empty
            return cfg;
        }

        JsonObject obj = config.as<JsonObject>();

        if (!obj.containsKey("screens")) {
            // No screens, return empty
            return cfg;
        }

        JsonArray screensArr = obj["screens"].as<JsonArray>();
        for (JsonVariant s : screensArr) {
            if (!s.is<JsonObject>()) continue;
            JsonObject scrObj = s.as<JsonObject>();

            std::string name = scrObj["name"] | "unnamed";
            std::string backScreen = scrObj["backScreen"] | "";

            auto screen = std::make_shared<Screen>(name, backScreen);

            // Parse components
            if (scrObj.containsKey("components")) {
                JsonArray compsArr = scrObj["components"].as<JsonArray>();
                for (JsonVariant c : compsArr) {
                    if (!c.is<JsonObject>()) continue;
                    JsonObject compObj = c.as<JsonObject>();

                    std::string uuid = compObj["uuid"] | "";
                    auto comp = std::make_shared<Component>(uuid);

                    // TODO: populate type and params
                    if (compObj.containsKey("type")) {
                        comp->type = compObj["type"].as<std::string>();
                    }
                    if (compObj.containsKey("params")) {
                        JsonObject paramsObj = compObj["params"].as<JsonObject>();
                        for (auto kv : paramsObj) {
                            comp->params[kv.key().c_str()] = kv.value().as<std::string>();
                        }
                    }

                    screen->addComponent(comp);
                }
            }

            // Parse modules
            if (scrObj.containsKey("modules")) {
                JsonArray modsArr = scrObj["modules"].as<JsonArray>();
                for (JsonVariant m : modsArr) {
                    if (!m.is<JsonObject>()) continue;
                    JsonObject modObj = m.as<JsonObject>();

                    std::string modName = modObj["name"] | "";
                    auto mod = std::make_shared<Module>(modName);

                    screen->addModule(mod);
                }
            }

            cfg.screens.push_back(screen);
        }

        return cfg;
    }
};
