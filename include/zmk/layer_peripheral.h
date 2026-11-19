/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

struct zmk_layer_peripheral_state {
    uint8_t index;
    const char *label;
};

struct zmk_layer_peripheral_state zmk_layer_peripheral_get_state(void);
void zmk_layer_peripheral_set_state(struct zmk_layer_peripheral_state state);
