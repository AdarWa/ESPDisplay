// spiffs_handler.h
#pragma once
#include <ArduinoJson.h>

// Mounts SPIFFS
bool spiffs_begin();

// Saves a JSON document to a file
bool spiffs_save_json(const char* path, const JsonDocument& doc);

// Loads a JSON document from a file
bool spiffs_load_json(const char* path, JsonDocument& doc);

// Removes a file from SPIFFS
bool spiffs_remove_file(const char* path);

// Checks if a file exists
bool spiffs_file_exists(const char* path);