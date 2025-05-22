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

static void close_warning_event_handler(lv_event_t* e);

lv_obj_t* create_warning_label(lv_obj_t* parent, const char* text) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(cont, 12, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0xFFF3CD), 0);  // Soft yellow
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 12, 0);
    lv_obj_set_style_shadow_width(cont, 8, 0);
    lv_obj_set_style_shadow_color(cont, lv_color_hex(0xE0A800), 0);
    lv_obj_set_style_shadow_opa(cont, LV_OPA_30, 0);

    // Use a flex layout with space between
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Sub-container for icon + text
    lv_obj_t* content = lv_obj_create(cont);
    lv_obj_remove_style_all(content);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // Warning icon
    lv_obj_t* icon = lv_label_create(content);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x856404), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);

    // Warning text
    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x856404), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_left(label, 8, 0);

    // Close (X) button
    lv_obj_t* close_btn = lv_btn_create(cont);
    lv_obj_set_size(close_btn, 32, 32);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xF5C6CB), 0); // light red/pink
    lv_obj_set_style_text_color(close_btn, lv_color_hex(0x721C24), 0);
    lv_obj_add_event_cb(close_btn, close_warning_event_handler, LV_EVENT_CLICKED, cont);

    // Add "X" symbol to button
    lv_obj_t* x_label = lv_label_create(close_btn);
    lv_label_set_text(x_label, LV_SYMBOL_CLOSE);
    lv_obj_center(x_label);

    return cont;
}

static void close_warning_event_handler(lv_event_t* e) {
    lv_obj_t* cont = (lv_obj_t*)lv_event_get_user_data(e); // warning container passed as user data
    lv_obj_del(cont); // delete the entire warning
}
