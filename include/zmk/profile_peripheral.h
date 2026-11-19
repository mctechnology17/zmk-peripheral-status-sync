/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct zmk_profile_peripheral_state {
    uint8_t index;
    bool connected;
    bool bonded;
};

struct zmk_profile_peripheral_state zmk_profile_peripheral_get_state(void);
void zmk_profile_peripheral_set_state(struct zmk_profile_peripheral_state state);
