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
#include <zmk/events/modifiers_peripheral_state_changed.h>
#include <zmk/modifiers_peripheral.h>

static uint8_t modifiers_peripheral_state;
static uint8_t last_modifiers_peripheral_state;

uint8_t zmk_modifiers_peripheral_get_state(void) { return modifiers_peripheral_state; }

void zmk_modifiers_peripheral_set_state(uint8_t state) {
    modifiers_peripheral_state = state;

    if (last_modifiers_peripheral_state != modifiers_peripheral_state) {
        LOG_DBG("Raised peripheral modifiers state changed 0x%02x", modifiers_peripheral_state);

        raise_zmk_modifiers_peripheral_state_changed(
            (struct zmk_modifiers_peripheral_state_changed){.state = modifiers_peripheral_state});

        last_modifiers_peripheral_state = modifiers_peripheral_state;
    }
}

static int modifiers_peripheral_init(void) {
    modifiers_peripheral_state = 0;
    last_modifiers_peripheral_state = 0;
    return 0;
}

SYS_INIT(modifiers_peripheral_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
