/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

#define ZMK_PERIPHERAL_STATUS_SYNC_VERSION 1

#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_WPM BIT(0)
#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_MODIFIERS BIT(1)
#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_PROFILE BIT(2)
#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_LAYER BIT(3)
#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_CENTRAL_BATTERY BIT(4)
#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_HID_INDICATORS BIT(5)
#define ZMK_PERIPHERAL_STATUS_SYNC_DIRTY_FORCE_FULL BIT(6)

#define ZMK_PERIPHERAL_STATUS_SYNC_BATTERY_UNKNOWN 255

struct zmk_peripheral_status_sync_state {
    uint8_t version;
    uint8_t dirty_mask;
    uint8_t wpm;
    uint8_t modifiers;
    uint8_t layer;
    uint8_t profile_index;
    bool profile_connected;
    bool profile_bonded;
    uint8_t central_battery_level;
    bool central_battery_charging;
    uint8_t hid_indicators;
    uint8_t sequence;
};

uint32_t zmk_peripheral_status_sync_pack_param1(struct zmk_peripheral_status_sync_state state);
uint32_t zmk_peripheral_status_sync_pack_param2(struct zmk_peripheral_status_sync_state state);
struct zmk_peripheral_status_sync_state zmk_peripheral_status_sync_unpack(uint32_t param1,
                                                                          uint32_t param2);

uint8_t zmk_peripheral_status_sync_get_central_battery(void);
bool zmk_peripheral_status_sync_get_central_battery_charging(void);
void zmk_peripheral_status_sync_set_central_battery(uint8_t level, bool charging);
uint8_t zmk_peripheral_status_sync_get_hid_indicators(void);
void zmk_peripheral_status_sync_set_hid_indicators(uint8_t indicators);
