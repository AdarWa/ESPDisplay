#ifndef CONFIG_H
#define CONFIG_H

// MACROS
#define set_black_text(obj) lv_obj_set_style_text_color(obj, lv_color_hex(COLORS_BLACK), LV_PART_MAIN)
// END OF MACROS

#define MAX_TEMP       30
#define MIN_TEMP       16
#define DEFAULT_TEMP   22

#define WELCOME_MSG    "Welcome!"

#define TRANSITION_TIME 100

#define FAN_LEVELS 6

// Time Synchronization
#define EPOCH_ZERO 1735686000 // in seconds 1-1-2025 01:00:00 GMT+2

//Components
#define AC_CONTROL 1
#define CLIMATE_CONTROL 1
#define FAN 1

//MQTT
#define MQTT_SERVER "192.168.1.32"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "RemoteControl"
#define MQTT_DEVICE_NAME "Display Remote Control"
#define MQTT_MANUFACTURER "Wasserman Inc."
#define MQTT_MODEL "ESP32"
#define MQTT_UPDATE_TOPIC "display/wass1/updates"

//Sleep
#define SLEEP_THRESHOLD 30000  // 30 seconds
#define ENABLE_SLEEP 1

//Colors
#define COLORS_GRAY 0xcccccc
#define COLORS_GREEN 0x4caf50
#define COLORS_RED 0xf44336
#define COLORS_ORANGE 0xffa500
#define COLORS_LIGHT_BLUE 0x4db6ac
#define COLORS_BLACK 0x000000
#define COLORS_SELECTED 0xBD9391

#endif // CONFIG_H
