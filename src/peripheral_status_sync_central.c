/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/peripheral_status_sync.h>
#include <zmk/split/central.h>
#include <zmk/split/transport/central.h>

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_WPM)
#include <zmk/events/wpm_state_changed.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_MODIFIERS)
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/events/usb_conn_state_changed.h>
#endif
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_LAYER)
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY) && IS_ENABLED(CONFIG_ZMK_BATTERY)
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#if !IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
#include <zmk/events/usb_conn_state_changed.h>
#endif
#endif
#endif
#define PERIPHERAL_STATUS_SYNC_BEHAVIOR_NAME "ussync"

extern const struct zmk_split_transport_central *active_transport;

static struct zmk_peripheral_status_sync_state pending_state = {
    .version = ZMK_PERIPHERAL_STATUS_SYNC_VERSION,
};
static struct zmk_peripheral_status_sync_state last_sent_state;
static struct k_work_delayable sync_work;
static struct k_work_delayable initial_sync_work;
static uint8_t initial_sync_attempts;

static struct zmk_behavior_binding_event status_sync_event(void) {
    return (struct zmk_behavior_binding_event){
        .position = 0,
        .timestamp = k_uptime_get(),
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
    };
}

static int status_sync_send_to_source(uint8_t source,
                                      struct zmk_peripheral_status_sync_state state) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = PERIPHERAL_STATUS_SYNC_BEHAVIOR_NAME,
        .param1 = zmk_peripheral_status_sync_pack_param1(state),
        .param2 = zmk_peripheral_status_sync_pack_param2(state),
    };

    return zmk_split_central_invoke_behavior(source, &binding, status_sync_event(), true);
}

static int status_sync_send_to_peripherals(struct zmk_peripheral_status_sync_state state) {
    if (state.dirty_mask == 0) {
        return 0;
    }

    bool synced = false;

    if (active_transport && active_transport->api &&
        active_transport->api->get_available_source_ids) {
        uint8_t source_ids[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
        int source_count = active_transport->api->get_available_source_ids(source_ids);
        if (source_count < 0) {
            return source_count;
        }

        for (int i = 0; i < source_count; i++) {
            int err = status_sync_send_to_source(source_ids[i], state);
            if (err) {
                LOG_DBG("Failed unified status sync to peripheral %d: %d", source_ids[i], err);
                continue;
            }
            synced = true;
        }

        return synced ? 0 : -ENOTCONN;
    }

    for (uint8_t source = 0; source < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; source++) {
        int err = status_sync_send_to_source(source, state);
        if (err) {
            LOG_DBG("Failed unified status sync to peripheral %d: %d", source, err);
            continue;
        }
        synced = true;
    }

    return synced ? 0 : -ENOTCONN;
}

static void status_sync_submit(bool force) {
    if (force) {
        pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_FORCE_FULL;
    }

    if (pending_state.dirty_mask == 0) {
        return;
    }

    pending_state.sequence = (pending_state.sequence + 1) & 0x07;
    struct zmk_peripheral_status_sync_state state = pending_state;

    if (!force && memcmp(&state, &last_sent_state, sizeof(state)) == 0) {
        pending_state.dirty_mask = 0;
        return;
    }

    int err = status_sync_send_to_peripherals(state);
    if (err) {
        LOG_DBG("Unified status sync failed: %d", err);
        return;
    }

    last_sent_state = state;
    pending_state.dirty_mask = 0;
}

static void status_sync_refresh_snapshot(void) {
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_MODIFIERS)
    pending_state.modifiers = zmk_hid_get_explicit_mods();
    pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_MODIFIERS;
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
    pending_state.profile_index = zmk_ble_active_profile_index();
    pending_state.profile_connected = zmk_ble_active_profile_is_connected();
    pending_state.profile_bonded = !zmk_ble_active_profile_is_open();
    pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_PROFILE;
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_LAYER)
    pending_state.layer = zmk_keymap_highest_layer_active();
    pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_LAYER;
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS)
    pending_state.hid_indicators = zmk_hid_indicators_get_current_profile();
    pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_HID_INDICATORS;
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY) && IS_ENABLED(CONFIG_ZMK_BATTERY)
    pending_state.central_battery_level = zmk_battery_state_of_charge();
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    pending_state.central_battery_charging = zmk_usb_is_powered();
#endif
    pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_CENTRAL_BATTERY;
#endif
}

static void status_sync_work_handler(struct k_work *work) { status_sync_submit(false); }

static void status_sync_schedule(void) {
    k_work_reschedule(&sync_work, K_MSEC(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INTERVAL_MS));
}

static void initial_sync_work_handler(struct k_work *work) {
    status_sync_refresh_snapshot();
    status_sync_submit(true);

    initial_sync_attempts++;
    if (initial_sync_attempts < CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INITIAL_ATTEMPTS) {
        k_work_schedule(&initial_sync_work,
                        K_MSEC(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INITIAL_RETRY_MS));
    }
}

static int peripheral_status_sync_central_listener(const zmk_event_t *eh) {
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_WPM)
    const struct zmk_wpm_state_changed *wpm_ev = as_zmk_wpm_state_changed(eh);
    if (wpm_ev != NULL) {
        if (pending_state.wpm != wpm_ev->state) {
            pending_state.wpm = wpm_ev->state;
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_WPM;
            status_sync_schedule();
        }
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_MODIFIERS)
    if (as_zmk_keycode_state_changed(eh) != NULL) {
        uint8_t modifiers = zmk_hid_get_explicit_mods();
        if (pending_state.modifiers != modifiers) {
            pending_state.modifiers = modifiers;
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_MODIFIERS;
            status_sync_schedule();
        }
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
    if (as_zmk_ble_active_profile_changed(eh) != NULL || as_zmk_endpoint_changed(eh) != NULL
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        || as_zmk_usb_conn_state_changed(eh) != NULL
#endif
    ) {
        pending_state.profile_index = zmk_ble_active_profile_index();
        pending_state.profile_connected = zmk_ble_active_profile_is_connected();
        pending_state.profile_bonded = !zmk_ble_active_profile_is_open();
        pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_PROFILE;
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY) &&                               \
    IS_ENABLED(CONFIG_ZMK_BATTERY) && IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        bool charging = zmk_usb_is_powered();
        if (pending_state.central_battery_charging != charging) {
            pending_state.central_battery_charging = charging;
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_CENTRAL_BATTERY;
        }
#endif
        status_sync_submit(true);
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_LAYER)
    if (as_zmk_layer_state_changed(eh) != NULL) {
        uint8_t layer = zmk_keymap_highest_layer_active();
        if (pending_state.layer != layer) {
            pending_state.layer = layer;
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_LAYER;
            status_sync_schedule();
        }
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS)
    const struct zmk_hid_indicators_changed *hid_ev = as_zmk_hid_indicators_changed(eh);
    if (hid_ev != NULL) {
        uint8_t indicators = hid_ev->indicators & 0x07;
        if (pending_state.hid_indicators != indicators) {
            pending_state.hid_indicators = indicators;
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_HID_INDICATORS;
            status_sync_schedule();
        }
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY) && IS_ENABLED(CONFIG_ZMK_BATTERY)
    const struct zmk_battery_state_changed *battery_ev = as_zmk_battery_state_changed(eh);
    if (battery_ev != NULL) {
        bool changed = pending_state.central_battery_level != battery_ev->state_of_charge;
        pending_state.central_battery_level = battery_ev->state_of_charge;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        bool charging = zmk_usb_is_powered();
        changed = changed || pending_state.central_battery_charging != charging;
        pending_state.central_battery_charging = charging;
#endif
        if (changed) {
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_CENTRAL_BATTERY;
            status_sync_schedule();
        }
        return ZMK_EV_EVENT_BUBBLE;
    }
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    if (as_zmk_usb_conn_state_changed(eh) != NULL) {
        bool charging = zmk_usb_is_powered();
        if (pending_state.central_battery_charging != charging) {
            pending_state.central_battery_charging = charging;
            pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_CENTRAL_BATTERY;
            status_sync_schedule();
        }
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
    if (as_zmk_position_state_changed(eh) != NULL) {
        pending_state.profile_index = zmk_ble_active_profile_index();
        pending_state.profile_connected = zmk_ble_active_profile_is_connected();
        pending_state.profile_bonded = !zmk_ble_active_profile_is_open();
        pending_state.dirty_mask |= ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_PROFILE;
        status_sync_submit(true);
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

static int peripheral_status_sync_central_init(void) {
    k_work_init_delayable(&sync_work, status_sync_work_handler);
    k_work_init_delayable(&initial_sync_work, initial_sync_work_handler);
    status_sync_refresh_snapshot();
    k_work_schedule(&initial_sync_work, K_MSEC(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_INITIAL_DELAY_MS));

    return 0;
}

SYS_INIT(peripheral_status_sync_central_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

ZMK_LISTENER(peripheral_status_sync_central, peripheral_status_sync_central_listener);
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_WPM)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_wpm_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_MODIFIERS)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_keycode_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_position_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_usb_conn_state_changed);
#endif
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_LAYER)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_layer_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_HID_INDICATORS)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_hid_indicators_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_CENTRAL_BATTERY) && IS_ENABLED(CONFIG_ZMK_BATTERY)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK) && !IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_SYNC_PROFILE)
ZMK_SUBSCRIPTION(peripheral_status_sync_central, zmk_usb_conn_state_changed);
#endif
#endif
