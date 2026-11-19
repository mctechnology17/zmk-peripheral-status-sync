/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <zmk/event_manager.h>

struct zmk_central_hid_indicators_peripheral_state_changed {
    uint8_t indicators;
};

ZMK_EVENT_DECLARE(zmk_central_hid_indicators_peripheral_state_changed);
