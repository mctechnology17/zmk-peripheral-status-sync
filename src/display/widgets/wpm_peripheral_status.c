/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/wpm_peripheral_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/wpm_peripheral_state_changed.h>
#include <zmk/wpm_peripheral.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct wpm_peripheral_status_state {
    uint8_t wpm;
};

static struct wpm_peripheral_status_state wpm_peripheral_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_peripheral_status_state){.wpm = zmk_wpm_peripheral_get_state()};
}

static void set_wpm_peripheral_symbol(lv_obj_t *label, struct wpm_peripheral_status_state state) {
    char text[4] = {};

    LOG_DBG("Peripheral WPM changed to %i", state.wpm);
    snprintf(text, sizeof(text), "%i", state.wpm);

    lv_label_set_text(label, text);
}

static void wpm_peripheral_status_update_cb(struct wpm_peripheral_status_state state) {
    struct zmk_widget_wpm_peripheral_status *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_wpm_peripheral_symbol(widget->obj, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_peripheral_status, struct wpm_peripheral_status_state,
                            wpm_peripheral_status_update_cb, wpm_peripheral_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_peripheral_status, zmk_wpm_peripheral_state_changed);

int zmk_widget_wpm_peripheral_status_init(struct zmk_widget_wpm_peripheral_status *widget,
                                          lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_label_set_text(widget->obj, "0");

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_peripheral_status_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_peripheral_status_obj(struct zmk_widget_wpm_peripheral_status *widget) {
    return widget->obj;
}
