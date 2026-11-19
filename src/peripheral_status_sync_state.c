/*
 * Copyright (c) 2026 Marcos Chow Castro
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>
#include <zmk/events/central_battery_peripheral_state_changed.h>
#include <zmk/events/central_hid_indicators_peripheral_state_changed.h>
#include <zmk/peripheral_status_sync.h>

#define P1_VERSION_SHIFT 0
#define P1_DIRTY_SHIFT 4
#define P1_WPM_SHIFT 11
#define P1_MODIFIERS_SHIFT 19
#define P1_LAYER_SHIFT 27

#define P2_PROFILE_INDEX_SHIFT 0
#define P2_PROFILE_CONNECTED_SHIFT 3
#define P2_PROFILE_BONDED_SHIFT 4
#define P2_CENTRAL_BATTERY_SHIFT 5
#define P2_CENTRAL_BATTERY_CHARGING_SHIFT 13
#define P2_HID_INDICATORS_SHIFT 14
#define P2_SEQUENCE_SHIFT 29

static uint8_t central_battery_level = ZMK_PERIPHERAL_STATUS_SYNC_BATTERY_UNKNOWN;
static bool central_battery_charging;
static uint8_t hid_indicators;

static uint8_t normalize_battery_level(uint8_t level) {
    return level <= 100 ? level : ZMK_PERIPHERAL_STATUS_SYNC_BATTERY_UNKNOWN;
}

uint32_t zmk_peripheral_status_sync_pack_param1(struct zmk_peripheral_status_sync_state state) {
    return ((uint32_t)(state.version & 0x0F) << P1_VERSION_SHIFT) |
           ((uint32_t)(state.dirty_mask & 0x7F) << P1_DIRTY_SHIFT) |
           ((uint32_t)state.wpm << P1_WPM_SHIFT) |
           ((uint32_t)state.modifiers << P1_MODIFIERS_SHIFT) |
           ((uint32_t)(state.layer & 0x1F) << P1_LAYER_SHIFT);
}

uint32_t zmk_peripheral_status_sync_pack_param2(struct zmk_peripheral_status_sync_state state) {
    return ((uint32_t)(state.profile_index & 0x07) << P2_PROFILE_INDEX_SHIFT) |
           (state.profile_connected ? BIT(P2_PROFILE_CONNECTED_SHIFT) : 0) |
           (state.profile_bonded ? BIT(P2_PROFILE_BONDED_SHIFT) : 0) |
           ((uint32_t)normalize_battery_level(state.central_battery_level)
            << P2_CENTRAL_BATTERY_SHIFT) |
           (state.central_battery_charging ? BIT(P2_CENTRAL_BATTERY_CHARGING_SHIFT) : 0) |
           ((uint32_t)(state.hid_indicators & 0x07) << P2_HID_INDICATORS_SHIFT) |
           ((uint32_t)(state.sequence & 0x07) << P2_SEQUENCE_SHIFT);
}

struct zmk_peripheral_status_sync_state zmk_peripheral_status_sync_unpack(uint32_t param1,
                                                                          uint32_t param2) {
    return (struct zmk_peripheral_status_sync_state){
        .version = (param1 >> P1_VERSION_SHIFT) & 0x0F,
        .dirty_mask = (param1 >> P1_DIRTY_SHIFT) & 0x7F,
        .wpm = (param1 >> P1_WPM_SHIFT) & 0xFF,
        .modifiers = (param1 >> P1_MODIFIERS_SHIFT) & 0xFF,
        .layer = (param1 >> P1_LAYER_SHIFT) & 0x1F,
        .profile_index = (param2 >> P2_PROFILE_INDEX_SHIFT) & 0x07,
        .profile_connected = (param2 & BIT(P2_PROFILE_CONNECTED_SHIFT)) != 0,
        .profile_bonded = (param2 & BIT(P2_PROFILE_BONDED_SHIFT)) != 0,
        .central_battery_level =
            normalize_battery_level((param2 >> P2_CENTRAL_BATTERY_SHIFT) & 0xFF),
        .central_battery_charging =
            (param2 & BIT(P2_CENTRAL_BATTERY_CHARGING_SHIFT)) != 0,
        .hid_indicators = (param2 >> P2_HID_INDICATORS_SHIFT) & 0x07,
        .sequence = (param2 >> P2_SEQUENCE_SHIFT) & 0x07,
    };
}

uint8_t zmk_peripheral_status_sync_get_central_battery(void) {
    return central_battery_level;
}

bool zmk_peripheral_status_sync_get_central_battery_charging(void) {
    return central_battery_charging;
}

void zmk_peripheral_status_sync_set_central_battery(uint8_t level, bool charging) {
    uint8_t normalized_level = normalize_battery_level(level);

    if (central_battery_level == normalized_level && central_battery_charging == charging) {
        return;
    }

    central_battery_level = normalized_level;
    central_battery_charging = charging;
    raise_zmk_central_battery_peripheral_state_changed(
        (struct zmk_central_battery_peripheral_state_changed){
            .level = central_battery_level, .charging = central_battery_charging});
}

uint8_t zmk_peripheral_status_sync_get_hid_indicators(void) {
    return hid_indicators;
}

void zmk_peripheral_status_sync_set_hid_indicators(uint8_t indicators) {
    uint8_t normalized_indicators = indicators & 0x07;

    if (hid_indicators == normalized_indicators) {
        return;
    }

    hid_indicators = normalized_indicators;
    raise_zmk_central_hid_indicators_peripheral_state_changed(
        (struct zmk_central_hid_indicators_peripheral_state_changed){
            .indicators = hid_indicators});
}
