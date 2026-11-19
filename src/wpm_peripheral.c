/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/wpm_peripheral_state_changed.h>
#include <zmk/wpm_peripheral.h>

#define WPM_PERIPHERAL_CHARS_PER_WORD 5.0

static uint8_t wpm_peripheral_state;
static uint8_t last_wpm_peripheral_state;
static uint16_t wpm_peripheral_update_counter;
static uint32_t position_release_count;

int zmk_wpm_peripheral_get_state(void) { return wpm_peripheral_state; }

void zmk_wpm_peripheral_set_state(uint8_t state) {
    wpm_peripheral_state = state;

    if (last_wpm_peripheral_state != wpm_peripheral_state) {
        LOG_DBG("Raised peripheral WPM state changed %d", wpm_peripheral_state);

        raise_zmk_wpm_peripheral_state_changed(
            (struct zmk_wpm_peripheral_state_changed){.state = wpm_peripheral_state});

        last_wpm_peripheral_state = wpm_peripheral_state;
    }
}

#if IS_ENABLED(CONFIG_ZMK_WPM_PERIPHERAL_SOURCE_LOCAL)
static int wpm_peripheral_event_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->source != ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!ev->state) {
        position_release_count++;
        LOG_DBG("peripheral position_release_count %d position %d", position_release_count,
                ev->position);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static void wpm_peripheral_work_handler(struct k_work *work) {
    wpm_peripheral_update_counter++;

    wpm_peripheral_state =
        (position_release_count / WPM_PERIPHERAL_CHARS_PER_WORD) /
        (wpm_peripheral_update_counter * CONFIG_ZMK_WPM_PERIPHERAL_UPDATE_INTERVAL_SECONDS / 60.0);

    zmk_wpm_peripheral_set_state(wpm_peripheral_state);

    if (wpm_peripheral_update_counter >= CONFIG_ZMK_WPM_PERIPHERAL_RESET_INTERVAL_SECONDS) {
        wpm_peripheral_update_counter = 0;
        position_release_count = 0;
    }
}

K_WORK_DEFINE(wpm_peripheral_work, wpm_peripheral_work_handler);

static void wpm_peripheral_expiry_function(struct k_timer *_timer) {
    k_work_submit(&wpm_peripheral_work);
}

K_TIMER_DEFINE(wpm_peripheral_timer, wpm_peripheral_expiry_function, NULL);
#endif

static int wpm_peripheral_init(void) {
    wpm_peripheral_state = 0;
    last_wpm_peripheral_state = 0;
    wpm_peripheral_update_counter = 0;
    position_release_count = 0;

#if IS_ENABLED(CONFIG_ZMK_WPM_PERIPHERAL_SOURCE_LOCAL)
    k_timer_start(&wpm_peripheral_timer,
                  K_SECONDS(CONFIG_ZMK_WPM_PERIPHERAL_UPDATE_INTERVAL_SECONDS),
                  K_SECONDS(CONFIG_ZMK_WPM_PERIPHERAL_UPDATE_INTERVAL_SECONDS));
#endif

    return 0;
}

#if IS_ENABLED(CONFIG_ZMK_WPM_PERIPHERAL_SOURCE_LOCAL)
ZMK_LISTENER(wpm_peripheral, wpm_peripheral_event_listener);
ZMK_SUBSCRIPTION(wpm_peripheral, zmk_position_state_changed);
#endif

SYS_INIT(wpm_peripheral_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
