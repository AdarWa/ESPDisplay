#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "esp_sleep.h"
#include "utils/utils.h"
#include "config.h"
#include "spiffs_handler.h"
#include "fonts/font_styles.h"
#include "battery_monitor.h"
#include "secrets.h"
#include "rpc/RPCSystem.hpp"
#include "renderer/ScreenRenderer.hpp"

// -------------------- Pins --------------------
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS
#define TOUCH_WAKEUP_PIN 36 // wakeup pin (T_IRQ)

// -------------------- Globals --------------------
boolean battery_present = true;
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

int x, y, z; // touch coordinates

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

RPCSystem rpcSystem(
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_BROKER,
    1883,
    MQTT_USER,
    MQTT_PASSWORD
);

ScreenRenderer renderer;

unsigned long last_touch_time = 0;
unsigned long init_millis = 0;
bool init_flag = false;
unsigned long lastLVGLTick = 0;

// -------------------- Utility --------------------
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void go_to_sleep() {
  #if ENABLE_SLEEP
  Serial.println("Going to sleep...");
  Serial.flush();
  delay(500);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0);
  esp_deep_sleep_start();
  #endif
}

// -------------------- Input --------------------
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
    last_touch_time = millis();
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// -------------------- Init Sections --------------------
void init_spiffs() {
  spiffs_begin();
}

void init_lvgl_display() {
  lv_init();
  lv_log_register_print_cb(log_print);

  // Touchscreen init
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(0);

  // Display init
  lv_display_t * disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

  // Input device
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Styles
  init_styles();
}

void init_battery_system() {
  if (!battery_init()) {
    #if ENABLE_BATTERY != 0
    Serial.println("Battery not detected. Please check wiring.");
    show_message_box("Battery not detected", "Please check wiring.");
    #endif
    battery_present = false;
  }
  battery_update();
}
void init_rpc_system() {
  if (!rpcSystem.begin()) {
    Serial.println("RPCSystem begin failed");
    show_message_box("Could not connect to RPC server", "Please check MQTT config");
    while (true) { delay(1000); }
  }
  ESP32RPC& rpc = rpcSystem.getRPC();
  JsonDocument params; // empty object
  JsonDocument res = rpc.call("get_config", params.as<JsonVariant>(), 5000);
  // If response didn't contain a result or timed out, res stays empty
  if (res.as<JsonVariantConst>().isNull()) {
    Serial.println("get_config returned null");
  } else {
    // Build screens from config
    renderer.buildFromConfig(res.as<JsonVariantConst>());
    renderer.showScreenById("scr1"); // default first screen
  }
}

// -------------------- Setup & Loop --------------------
void setup() {
  Serial.begin(115200);
  Serial.printf("LVGL Library Version: %d.%d.%d\n",
                lv_version_major(), lv_version_minor(), lv_version_patch());

  pinMode(TOUCH_WAKEUP_PIN, INPUT);

  init_spiffs();
  init_lvgl_display();
  init_rpc_system();

  init_millis = millis();
  last_touch_time = millis();
}

void loop() {
  rpcSystem.getRPC().loop();
  lv_timer_handler();  
  delay(5);

  if (!init_flag) {
    if (battery_present || ENABLE_BATTERY == 0)
      close_message_box();
    init_flag = true;
  }

  unsigned long now = millis();
  lv_tick_inc(now - lastLVGLTick);
  lastLVGLTick = now;

  if ((now - last_touch_time) > SLEEP_THRESHOLD) {
    go_to_sleep();
  }
}
