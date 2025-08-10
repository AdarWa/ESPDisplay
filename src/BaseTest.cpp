#include "BaseTest.h"
#include <lvgl.h>

BaseTest::BaseTest()
    : AdvancedPage("My Page") {}

void BaseTest::setup() {
    AdvancedPage::setup(); // Init modules
    LV_LOG_USER("Base Test Setup");
    lv_obj_t* btn = lv_btn_create(getRoot());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me");
}
