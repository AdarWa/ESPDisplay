#include "ScreenRenderer.hpp"
#include <cstring>
#include <cstdlib>

// ------------- ScreenRenderer -------------

void ScreenRenderer::ensureRegistrySetup() {
    static bool inited = false;
    if (inited) return;
    inited = true;

    ComponentRegistry::instance().registerType("light", [](){
        return new LightComponent();
    });
}

void ScreenRenderer::buildFromConfig(JsonVariantConst cfg) {
    ensureRegistrySetup();
    screens.clear();

    if (!cfg["screens"].is<JsonArrayConst>()) return;
    JsonArrayConst arr = cfg["screens"].as<JsonArrayConst>();

    for (JsonVariantConst s : arr) {
        ScreenInfo info;
        info.scr_id = String(s["scr_id"] | "");
        info.name = String(s["name"] | "");
        info.back_screen = String(s["back_screen"] | "");

        // root container for this screen
        lv_obj_t* root = lv_obj_create(lv_scr_act());
        lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_all(root, 10, 0);
        lv_obj_add_flag(root, LV_OBJ_FLAG_SCROLLABLE);

        // back button if needed
        if (!info.back_screen.isEmpty()) {
            lv_obj_t* back = lv_btn_create(root);
            lv_obj_t* backlbl = lv_label_create(back);
            lv_label_set_text(backlbl, "<");

            BackCbData* ud = (BackCbData*)malloc(sizeof(BackCbData));
            if (ud) {
                ud->self = this;
                const char* tgt = info.back_screen.c_str();
                size_t len = strlen(tgt);
                ud->target = (char*)malloc(len + 1);
                if (ud->target) {
                    memcpy(ud->target, tgt, len + 1);

                    lv_obj_add_event_cb(back, [](lv_event_t* e){
                        BackCbData* d = (BackCbData*)lv_event_get_user_data(e);
                        if (!d || !d->self || !d->target) return;
                        d->self->showScreenById(String(d->target));
                    }, LV_EVENT_CLICKED, ud);

                    // Cleanup when the button is deleted
                    lv_obj_add_event_cb(back, [](lv_event_t* e){
                        BackCbData* d = (BackCbData*)lv_event_get_user_data(e);
                        if (!d) return;
                        if (d->target) free(d->target);
                        free(d);
                    }, LV_EVENT_DELETE, ud);
                } else {
                    free(ud);
                }
            }
        }

        // components container
        lv_obj_t* grid = lv_obj_create(root);
        lv_obj_set_width(grid, LV_PCT(100));
        lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_all(grid, 8, 0);

        // build components
        if (s["components"].is<JsonArrayConst>()) {
            for (JsonVariantConst c : s["components"].as<JsonArrayConst>()) {
                CompCtx ctx;
                ctx.comp_id = String(c["comp_id"] | "");
                ctx.type    = String(c["type"] | "");
                ctx.params  = c["params"];

                std::unique_ptr<IComponent> comp(
                    ComponentRegistry::instance().create(ctx.type)
                );

                if (!comp) {
                    lv_obj_t* unknown = lv_label_create(grid);
                    String t = "Unknown component: " + ctx.type;
                    lv_label_set_text(unknown, t.c_str());
                } else {
                    comp->build(grid, ctx);
                }
            }
        }

        info.root = root;
        screens[info.scr_id] = info;
        lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    }
}

void ScreenRenderer::showScreenById(const String& scr_id) {
    for (auto &it : screens) {
        if (it.second.root) lv_obj_add_flag(it.second.root, LV_OBJ_FLAG_HIDDEN);
    }
    auto it = screens.find(scr_id);
    if (it == screens.end()) return;
    lv_obj_clear_flag(it->second.root, LV_OBJ_FLAG_HIDDEN);
}
