/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/peripheral_status_sync.h>

#if IS_ENABLED(CONFIG_ZMK_WPM_PERIPHERAL_SOURCE_SYNCED)
#include <zmk/wpm_peripheral.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_MODIFIERS_PERIPHERAL_SOURCE_SYNCED)
#include <zmk/modifiers_peripheral.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_PROFILE_PERIPHERAL_SOURCE_SYNCED)
#include <zmk/profile_peripheral.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_LAYER_PERIPHERAL_SOURCE_SYNCED)
#include <zmk/layer_peripheral.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY)
#include <zmk/peripheral_status_sync.h>
#endif

#define PERIPHERAL_STATUS_SYNC_BEHAVIOR_NAME "ussync"

static int behavior_peripheral_status_sync_init(const struct device *dev) { return 0; }

static bool status_sync_should_apply(struct zmk_peripheral_status_sync_state state, uint8_t bit) {
    return (state.dirty_mask & (bit | ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_FORCE_FULL)) != 0;
}

static int on_peripheral_status_sync_pressed(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    struct zmk_peripheral_status_sync_state state =
        zmk_peripheral_status_sync_unpack(binding->param1, binding->param2);

    if (state.version != ZMK_PERIPHERAL_STATUS_SYNC_VERSION) {
        LOG_DBG("Ignoring unsupported peripheral status sync version %d", state.version);
        return ZMK_BEHAVIOR_OPAQUE;
    }

#if IS_ENABLED(CONFIG_ZMK_WPM_PERIPHERAL_SOURCE_SYNCED)
    if (status_sync_should_apply(state, ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_WPM)) {
        zmk_wpm_peripheral_set_state(state.wpm);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_MODIFIERS_PERIPHERAL_SOURCE_SYNCED)
    if (status_sync_should_apply(state, ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_MODIFIERS)) {
        zmk_modifiers_peripheral_set_state(state.modifiers);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PROFILE_PERIPHERAL_SOURCE_SYNCED)
    if (status_sync_should_apply(state, ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_PROFILE)) {
        zmk_profile_peripheral_set_state((struct zmk_profile_peripheral_state){
            .index = state.profile_index,
            .connected = state.profile_connected,
            .bonded = state.profile_bonded,
        });
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_LAYER_PERIPHERAL_SOURCE_SYNCED)
    if (status_sync_should_apply(state, ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_LAYER)) {
        zmk_layer_peripheral_set_state((struct zmk_layer_peripheral_state){.index = state.layer});
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY)
    if (status_sync_should_apply(state, ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_CENTRAL_BATTERY)) {
        zmk_peripheral_status_sync_set_central_battery(state.central_battery_level,
                                                       state.central_battery_charging);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS)
    if (status_sync_should_apply(state, ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_HID_INDICATORS)) {
        zmk_peripheral_status_sync_set_hid_indicators(state.hid_indicators);
    }
#endif

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_peripheral_status_sync_released(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_peripheral_status_sync_driver_api = {
    .binding_pressed = on_peripheral_status_sync_pressed,
    .binding_released = on_peripheral_status_sync_released,
};

DEVICE_DEFINE(behavior_peripheral_status_sync, PERIPHERAL_STATUS_SYNC_BEHAVIOR_NAME,
              behavior_peripheral_status_sync_init, NULL, NULL, NULL, POST_KERNEL,
              CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
              &behavior_peripheral_status_sync_driver_api);

static const STRUCT_SECTION_ITERABLE(zmk_behavior_ref, zmk_behavior_peripheral_status_sync_ref) =
    {
        .device = DEVICE_GET(behavior_peripheral_status_sync),
        .metadata = {},
};
