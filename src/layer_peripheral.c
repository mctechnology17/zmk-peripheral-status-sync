/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/layer_peripheral_state_changed.h>
#include <zmk/layer_peripheral.h>

static struct zmk_layer_peripheral_state layer_peripheral_state;
static struct zmk_layer_peripheral_state last_layer_peripheral_state;

#if IS_ENABLED(CONFIG_ZMK_LAYER_PERIPHERAL_LABELS)
static const char *const layer_peripheral_labels[] = {
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_0,  CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_1,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_2,  CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_3,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_4,  CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_5,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_6,  CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_7,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_8,  CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_9,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_10, CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_11,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_12, CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_13,
    CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_14, CONFIG_ZMK_LAYER_PERIPHERAL_LABEL_15,
};
#endif

static const char *layer_peripheral_label_for_index(uint8_t index) {
#if IS_ENABLED(CONFIG_ZMK_LAYER_PERIPHERAL_LABELS)
    if (index < ARRAY_SIZE(layer_peripheral_labels) && layer_peripheral_labels[index][0] != '\0') {
        return layer_peripheral_labels[index];
    }
#endif

    return NULL;
}

struct zmk_layer_peripheral_state zmk_layer_peripheral_get_state(void) {
    return layer_peripheral_state;
}

void zmk_layer_peripheral_set_state(struct zmk_layer_peripheral_state state) {
    if (state.label == NULL) {
        state.label = layer_peripheral_label_for_index(state.index);
    }

    layer_peripheral_state = state;

    if (last_layer_peripheral_state.index != layer_peripheral_state.index ||
        last_layer_peripheral_state.label != layer_peripheral_state.label) {
        LOG_DBG("Raised peripheral layer state changed index=%d", layer_peripheral_state.index);

        raise_zmk_layer_peripheral_state_changed(
            (struct zmk_layer_peripheral_state_changed){
                .index = layer_peripheral_state.index,
                .label = layer_peripheral_state.label,
            });

        last_layer_peripheral_state = layer_peripheral_state;
    }
}

static int layer_peripheral_init(void) {
    layer_peripheral_state = (struct zmk_layer_peripheral_state){0};
    last_layer_peripheral_state = (struct zmk_layer_peripheral_state){0};
    return 0;
}

SYS_INIT(layer_peripheral_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
