#pragma once
#include "RPCSystem.hpp"
#include <config_manager.h>

void register_config(ESP32RPC& rpc){
    JsonDocument configDoc = rpc.call("get_config", JsonVariant());
    if (configDoc.as<JsonVariantConst>().isNull()) return;

    String configStr;
    serializeJson(configDoc, configStr);
    LV_LOG_USER("Config: %s", configStr.c_str());

    ConfigManager::getInstance().setConfig(
        std::make_unique<Config>(Config::parseConfig(configDoc.as<JsonVariant>()))
    );
}

void init_rpc_handlers(ESP32RPC& rpc){
    register_config(rpc);
}
