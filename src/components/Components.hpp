#pragma once
#include "renderer/ScreenRenderer.hpp"

class LightComponent : public IComponent {
public:
    lv_obj_t* build(lv_obj_t* parent, const CompCtx& ctx) override;
};