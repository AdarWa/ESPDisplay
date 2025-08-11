#include "Screen.h"
#include <config.h>
#include "ScreenManager.h"

Screen::Screen(const std::string& name, std::string backScreen)
    : name_(name), backScreen_(backScreen) {
        parent = lv_obj_create(nullptr);
    }

const std::string& Screen::getName() const {
    return name_;
}

std::string Screen::getBackScreen() const {
    return backScreen_;
}

void Screen::setBackScreen(std::string backName) {
    backScreen_ = backName;
}

void Screen::addComponent(std::shared_ptr<Component> comp) {
    components_.push_back(comp);
}

void Screen::addModule(std::shared_ptr<Module> mod) {
    modules_.push_back(mod);
}

const std::vector<std::shared_ptr<Component>>& Screen::getComponents() const {
    return components_;
}

const std::vector<std::shared_ptr<Module>>& Screen::getModules() const {
    return modules_;
}

void Screen::onLoad() {
    for (auto& mod : modules_) mod->onLoad();
    for (auto& comp : components_) comp->onLoad();
}

void Screen::onUpdate() {
    for (auto& comp : components_) comp->onUpdate();
    for (auto& mod : modules_) mod->onUpdate();
}

void Screen::onUnload() {
    for (auto& comp : components_) comp->onUnload();
    for (auto& mod : modules_) mod->onUnload();
}

lv_obj_t* Screen::render(){
    lv_obj_clean(parent);

    if (!backScreen_.empty()) {
        lv_obj_t* back_btn = lv_btn_create(parent);
        lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), 0);

        lv_obj_t* label = lv_label_create(back_btn);
        lv_label_set_text(label, "<");

        lv_obj_set_user_data(back_btn, this);
        lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
            auto screen = static_cast<Screen*>(lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e)));
            ScreenManager::getInstance().switchTo(screen->getBackScreen());
        }, LV_EVENT_CLICKED, this);
    }

    for (auto& comp : components_) {
        comp->render(parent);
    }
    return parent;
}