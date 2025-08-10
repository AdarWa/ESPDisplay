#include "PageManager.h"
#include <config.h>

PageManager& PageManager::getInstance() {
    static PageManager instance;
    return instance;
}

void PageManager::registerPage(const std::string& name, std::shared_ptr<Page> page) {
    pages[name] = page;
}

void PageManager::loadPage(const std::string& name) {
    auto it = pages.find(name);
    if (it != pages.end()) {
        lastPage = currentPage;
        currentPage = it->second;
        loadCurrentPage();
    }
}

void PageManager::loadCurrentPage(){
    currentPage->onLoad();

    lv_obj_t* scr = lv_obj_create(nullptr);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);

    lv_obj_t *btn_advanced = lv_btn_create(scr);
    lv_obj_align(btn_advanced, LV_ALIGN_TOP_RIGHT, 0, 0); // Top-right with padding
    lv_obj_set_size(btn_advanced, 80, 25);
    lv_obj_set_user_data(btn_advanced, this);
    lv_obj_add_event_cb(btn_advanced, [](lv_event_t* e) {
        PageManager* pm = (PageManager*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
        // pm->currentPage->renderAdvancedSettings
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn_advanced, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Label for button
    lv_obj_t *label_adv = lv_label_create(btn_advanced);
    lv_label_set_text(label_adv, "Advanced");
    lv_obj_center(label_adv);
    set_black_text(label_adv);

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_user_data(back_btn, this);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        PageManager* pm = (PageManager*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
        pm->backPage();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    set_black_text(back_label);
}

void PageManager::update() {
    currentPage->update();
}

void PageManager::backPage(){
    currentPage = lastPage;
    loadCurrentPage();

}