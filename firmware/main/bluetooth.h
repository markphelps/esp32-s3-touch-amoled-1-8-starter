// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    // NimBLE invokes these on its host task. Do not block or call LVGL here.
    // ready runs again after host recovery; discard old connections on reset.
    void (*ready)(uint8_t own_address_type);
    void (*reset)(int reason);
} app_bluetooth_callbacks_t;

// Call once after NVS initialization. The callbacks are copied, and ready is
// required. This module owns NimBLE initialization and the host task for clients.
esp_err_t app_bluetooth_start(const app_bluetooth_callbacks_t *callbacks);
