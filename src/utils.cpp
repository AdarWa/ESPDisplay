#include "utils.h"

static lv_obj_t* g_msgbox_bg = NULL;

// Close the message box (can be called from event handler or externally)
void close_message_box() {
    if (g_msgbox_bg != NULL) {
        lv_obj_del(g_msgbox_bg);
        g_msgbox_bg = NULL;
    }
}

// Event callback for button clicks
static void msgbox_event_cb(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    const char* btn_txt = lv_label_get_text(label);

    LV_LOG_USER("Button %s clicked", btn_txt);

    if (strcmp(btn_txt, "Cancel") == 0) {
        close_message_box();  // Close the box when "Cancel" is clicked
    }
    else if (strcmp(btn_txt, "Apply") == 0) {
        // Do something on Apply...
        LV_LOG_USER("Apply logic here.");
        close_message_box();  // Optional: close after Apply
    }
}

void show_message_box(const char* title, const char* message) {
    if (g_msgbox_bg != NULL) {
        close_message_box();  // Remove existing box before showing a new one
    }

    g_msgbox_bg = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(g_msgbox_bg);  // Remove default styles
    lv_obj_set_size(g_msgbox_bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_msgbox_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_msgbox_bg, LV_OPA_50, 0);  // 50% transparent
    lv_obj_clear_flag(g_msgbox_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *g_msgbox = lv_msgbox_create(g_msgbox_bg);
    lv_msgbox_add_title(g_msgbox, title);
    lv_msgbox_add_text(g_msgbox, message);

    lv_obj_t *btn = lv_msgbox_add_footer_button(g_msgbox, "Cancel");
    lv_obj_add_event_cb(btn, msgbox_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_center(g_msgbox);  // Center on screen
}
