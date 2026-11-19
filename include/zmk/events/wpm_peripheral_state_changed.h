/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/wpm_peripheral.h>

struct zmk_wpm_peripheral_state_changed {
    int state;
};

ZMK_EVENT_DECLARE(zmk_wpm_peripheral_state_changed);
