/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/modifiers_peripheral.h>

struct zmk_modifiers_peripheral_state_changed {
    uint8_t state;
};

ZMK_EVENT_DECLARE(zmk_modifiers_peripheral_state_changed);
