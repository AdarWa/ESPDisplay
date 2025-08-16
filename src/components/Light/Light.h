#pragma once
#include "base/component/Component.h"
#include <iostream>

class Light : public Component {
public:
    Light(const std::string& uuid)
        : Component(uuid) {}

    lv_obj_t* render(lv_obj_t* parent) override;

    void onLoad() override;

    void onUpdate() override;

    void onUnload() override;
};
