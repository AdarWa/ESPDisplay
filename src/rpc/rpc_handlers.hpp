#include "ESP32RPC.hpp"
#include <config_manager.h>

void register_config(ESP32RPC& rpc){
    JsonVariant configVar = rpc.call("get_config", JsonVariant());
    if (configVar.isNull()) return;

    DynamicJsonDocument doc(CONFIG_BUFFER_SIZE);
    doc.set(configVar);

    String configStr;
    serializeJson(doc, configStr);
    LV_LOG_USER("Config: %s", configStr.c_str());

    ConfigManager::getInstance().setConfig(
        std::make_unique<Config>(Config::parseConfig(doc.as<JsonVariant>()))
    );
}

void init_rpc_handlers(ESP32RPC& rpc){
    register_config(rpc);
}