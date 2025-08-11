#pragma once
#include "base/component/Component.h"
#include <iostream>

class TestComponent : public Component {
public:
    TestComponent(const std::string& uuid)
        : Component(uuid) {}

    lv_obj_t* render(lv_obj_t* parent) override {
        std::cout << "Rendering TestComponent with UUID: " << uuid_ << std::endl;
        return lv_obj_create(parent);
    }

    void onLoad() override {
        std::cout << "Loading TestComponent with UUID: " << uuid_ << std::endl;
    }

    void onUpdate() override {
        std::cout << "Updating TestComponent with UUID: " << uuid_ << std::endl;
    }

    void onUnload() override {
        std::cout << "Unloading TestComponent with UUID: " << uuid_ << std::endl;
    }
};
