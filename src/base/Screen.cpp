#include "Screen.h"

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
    for (auto& comp : components_) {
        comp->render(parent);
    }
    return parent;
}