/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zmk/event_manager.h>

struct zmk_central_battery_peripheral_state_changed {
    uint8_t level;
    bool charging;
};

ZMK_EVENT_DECLARE(zmk_central_battery_peripheral_state_changed);
