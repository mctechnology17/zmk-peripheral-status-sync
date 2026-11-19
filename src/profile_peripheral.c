/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if IS_ENABLED(CONFIG_ZMK_PROFILE_PERIPHERAL_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/profile_peripheral_state_changed.h>
#include <zmk/profile_peripheral.h>

static struct zmk_profile_peripheral_state profile_peripheral_state;
static struct zmk_profile_peripheral_state last_profile_peripheral_state;

static bool profile_peripheral_state_equal(struct zmk_profile_peripheral_state a,
                                           struct zmk_profile_peripheral_state b) {
    return a.index == b.index && a.connected == b.connected && a.bonded == b.bonded;
}

struct zmk_profile_peripheral_state zmk_profile_peripheral_get_state(void) {
    return profile_peripheral_state;
}

static void profile_peripheral_set_state(struct zmk_profile_peripheral_state state, bool persist) {
    profile_peripheral_state = state;

    if (!profile_peripheral_state_equal(last_profile_peripheral_state, profile_peripheral_state)) {
        LOG_DBG("Raised peripheral profile state changed index=%d connected=%d bonded=%d",
                profile_peripheral_state.index, profile_peripheral_state.connected,
                profile_peripheral_state.bonded);

        raise_zmk_profile_peripheral_state_changed(
            (struct zmk_profile_peripheral_state_changed){
                .index = profile_peripheral_state.index,
                .connected = profile_peripheral_state.connected,
                .bonded = profile_peripheral_state.bonded,
            });

        last_profile_peripheral_state = profile_peripheral_state;

#if IS_ENABLED(CONFIG_ZMK_PROFILE_PERIPHERAL_SETTINGS)
        if (persist) {
            int err = settings_save_one("zmk_profile_peripheral/state",
                                        &profile_peripheral_state,
                                        sizeof(profile_peripheral_state));
            if (err) {
                LOG_DBG("Failed to persist peripheral profile state: %d", err);
            }
        }
#endif
    }
}

void zmk_profile_peripheral_set_state(struct zmk_profile_peripheral_state state) {
    profile_peripheral_set_state(state, true);
}

static int profile_peripheral_init(void) {
    profile_peripheral_state = (struct zmk_profile_peripheral_state){0};
    last_profile_peripheral_state = (struct zmk_profile_peripheral_state){0};
    return 0;
}

SYS_INIT(profile_peripheral_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#if IS_ENABLED(CONFIG_ZMK_PROFILE_PERIPHERAL_SETTINGS)

static int profile_peripheral_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                           void *cb_arg) {
    if (!settings_name_steq(name, "state", NULL)) {
        return -ENOENT;
    }

    if (len != sizeof(struct zmk_profile_peripheral_state)) {
        LOG_ERR("Invalid peripheral profile state size: %d", len);
        return -EINVAL;
    }

    struct zmk_profile_peripheral_state state;
    int err = read_cb(cb_arg, &state, sizeof(state));
    if (err <= 0) {
        LOG_ERR("Failed to read peripheral profile state: %d", err);
        return err;
    }

    profile_peripheral_set_state(state, false);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(profile_peripheral, "zmk_profile_peripheral", NULL,
                               profile_peripheral_settings_set, NULL, NULL);

#endif
