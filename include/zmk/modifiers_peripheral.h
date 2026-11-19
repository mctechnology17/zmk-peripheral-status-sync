/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

uint8_t zmk_modifiers_peripheral_get_state(void);
void zmk_modifiers_peripheral_set_state(uint8_t state);
