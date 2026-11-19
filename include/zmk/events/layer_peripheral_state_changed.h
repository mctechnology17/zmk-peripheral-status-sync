/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <zmk/event_manager.h>

struct zmk_layer_peripheral_state_changed {
    uint8_t index;
    const char *label;
};

ZMK_EVENT_DECLARE(zmk_layer_peripheral_state_changed);
