/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zmk/event_manager.h>

struct zmk_profile_peripheral_state_changed {
    uint8_t index;
    bool connected;
    bool bonded;
};

ZMK_EVENT_DECLARE(zmk_profile_peripheral_state_changed);
