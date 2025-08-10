#pragma once
#include <lvgl.h>

class Module {
public:
    virtual ~Module() = default;

    // Called when page is loaded
    virtual void onLoad() = 0;

    // Called when page is unloaded
    virtual void onUnload() = 0;

    // Update module (e.g. timer tick)
    virtual void update() = 0;

    // Render the module's settings on the advanced screen
    virtual void renderSettings(lv_obj_t* parent) = 0;
};