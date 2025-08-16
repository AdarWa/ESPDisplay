#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>
#include "esp_sleep.h"
#include "utils/utils.h"
#include "config.h"
#include "spiffs_handler.h"
#include "fonts/font_styles.h"
#include "battery_monitor.h"
#include "screen_registrer.h"
#include "secrets.h"
#include "rpc/RPCSystem.hpp"

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

boolean battery_present = true;
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define TOUCH_WAKEUP_PIN 36    // T_IRQ pin (used as wake-up source)

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];
RPCSystem rpc(
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_BROKER,
    1883,
    MQTT_USER,      // optional
    MQTT_PASSWORD   // optional
);


// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

unsigned long last_touch_time = 0;

void go_to_sleep() {
  #if ENABLE_SLEEP
  Serial.println("Going to sleep...");
  Serial.flush();
  delay(500); // Let serial print
  // Configure external wakeup on falling edge of IO36
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0);
  esp_deep_sleep_start();  // Enter deep sleep
  #endif
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    last_touch_time = millis();

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    /* Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();*/
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}


unsigned long init_millis = 0;
bool init_flag = false;

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  pinMode(TOUCH_WAKEUP_PIN, INPUT);
  spiffs_begin();
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(0);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
    
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);
  init_styles();

  // init the battery gauge
  if(!battery_init()){
    #if ENABLE_BATTERY != 0
    Serial.println("Battery not detected. Please check wiring.");
    show_message_box("Battery not detected", "Please check wiring.");
    #endif
    battery_present = false;
  }
  battery_update();

  register_screens();
  ScreenManager::getInstance().switchTo("TestScreen");
  if(!rpc.begin()){
    show_message_box("Could not connect to RPC server", "Please check MQTT config");
  }

  LV_LOG_USER("Device UUID: %d", rpc.getRPC().getUUID());

  rpc.getRPC().registerMethod("add", [](JsonVariant params) -> JsonVariant {
    int a = params["a"] | 0;
    int b = params["b"] | 0;

    // Create a static DynamicJsonDocument to hold the result object
    static DynamicJsonDocument doc(64); 
    doc.clear(); // clear previous content

    // Create the result object
    doc["result"] = a+b;

    return doc.as<JsonVariant>();
  });
  static DynamicJsonDocument doc(64);
  doc.clear();
  doc["a"] = 5;
  doc["b"] = 10;
  LV_LOG_USER("%d",rpc.getRPC().call("add", doc.as<JsonVariant>()).as<int>());

  
  // Function to draw the GUI (text, buttons and sliders)
    if(battery_present || ENABLE_BATTERY == 0) {
      show_message_box("Connecting to WiFi...", "...");
    }
    init_millis = millis();
    last_touch_time = millis();


}

unsigned long lastLVGLTick = 0;

void loop() {
    rpc.getRPC().loop();
    // Call lv_timer_handler often
    lv_timer_handler();  // required for LVGL to render and handle input

    delay(5);
      if(!init_flag){
        // Initialize WiFi and MQTT connection
      }
      if(!init_flag){
        if(battery_present || ENABLE_BATTERY == 0)
          close_message_box();
        init_flag=true;
      }
    unsigned long now = millis();
    lv_tick_inc(now - lastLVGLTick);
    lastLVGLTick = now;

    if ((now - last_touch_time) > SLEEP_THRESHOLD) {
      go_to_sleep();
    }
}