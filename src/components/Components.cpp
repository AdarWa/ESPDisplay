#include "Components.hpp"
#include "rpc/RPCSystem.hpp"

extern RPCSystem rpcSystem;

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
    struct LightCbData { 
        char* comp_id;
    };
    LightCbData* ud = (LightCbData*)malloc(sizeof(LightCbData));
    if (ud) {
        const char* cid = ctx.comp_id.c_str();
        size_t len = strlen(cid);
        ud->comp_id = (char*)malloc(len + 1);
        if (ud->comp_id) memcpy(ud->comp_id, cid, len + 1);
        else { free(ud); ud = nullptr; }
    }

    lv_obj_add_event_cb(btn, [](lv_event_t* e){
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* lbl = lv_obj_get_child(btn, 0);
        const char* cur = lv_label_get_text(lbl);
        bool on = strcmp(cur, "ON") == 0;
        bool new_state = !on;
        lv_label_set_text(lbl, new_state ? "ON" : "OFF");

        // Inform server via RPC (non-blocking-ish: zero timeout)
        JsonDocument params;
        params["comp_id"] = ((LightCbData*)lv_event_get_user_data(e)) && ((LightCbData*)lv_event_get_user_data(e))->comp_id
                              ? ((LightCbData*)lv_event_get_user_data(e))->comp_id : "";
        JsonVariant stateDict = params["state"].to<JsonVariant>();
        stateDict["power"] = new_state ? "on" : "off";
        rpcSystem.getRPC().call("update_state", params.as<JsonVariant>(), 0);
    }, LV_EVENT_CLICKED, ud);

    // Cleanup user data when button is deleted
    if (ud) {
        lv_obj_add_event_cb(btn, [](lv_event_t* e){
            LightCbData* d = (LightCbData*)lv_event_get_user_data(e);
            if (!d) return;
            if (d->comp_id) free(d->comp_id);
            free(d);
        }, LV_EVENT_DELETE, ud);
    }

    return parent;
}
