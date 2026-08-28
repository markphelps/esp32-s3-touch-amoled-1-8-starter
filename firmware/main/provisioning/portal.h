// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app_esp_err.h"

#define APP_PORTAL_AP_NAME_CAPACITY 15
#define APP_PORTAL_AP_PASSWORD_CAPACITY 11
#define APP_PORTAL_QR_PAYLOAD_CAPACITY 64

typedef struct {
    char ap_name[APP_PORTAL_AP_NAME_CAPACITY];
    char ap_password[APP_PORTAL_AP_PASSWORD_CAPACITY];
    char qr_payload[APP_PORTAL_QR_PAYLOAD_CAPACITY];
} app_portal_config_t;

esp_err_t app_portal_prepare(app_portal_config_t *configuration);
esp_err_t app_portal_start(const app_portal_config_t *configuration);
