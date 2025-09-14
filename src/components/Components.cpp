#include "Components.hpp"

// ------------- LightComponent -------------

lv_obj_t* LightComponent::build(lv_obj_t* parent, const CompCtx& ctx) {
    const char* label = ctx.params["label"] | "Light";
    bool initial = ctx.params["initial"] | false;

    // toggle button
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_t* txt = lv_label_create(btn);
    lv_label_set_text(txt, initial ? "ON" : "OFF");
    lv_obj_center(txt);

    // caption label
    lv_obj_t* cap = lv_label_create(parent);
    lv_label_set_text(cap, label);

    // button callback
    lv_obj_add_event_cb(btn, [](lv_event_t* e){
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* lbl = lv_obj_get_child(btn, 0);
        const char* cur = lv_label_get_text(lbl);
        bool on = strcmp(cur, "ON") == 0;
        lv_label_set_text(lbl, on ? "OFF" : "ON");
        // TODO: RPC call can go here to inform server
    }, LV_EVENT_CLICKED, nullptr);

    return parent;
}