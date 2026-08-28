// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app_esp_err.h"

#define APP_WIFI_SSID_CAPACITY 33
#define APP_WIFI_PASSWORD_CAPACITY 65

typedef struct {
    char wifi_ssid[APP_WIFI_SSID_CAPACITY];
    char wifi_password[APP_WIFI_PASSWORD_CAPACITY];
} app_credentials_t;

typedef enum {
    APP_PROVISION_NONE,
    APP_PROVISION_COMPLETE,
} app_provision_state_t;

esp_err_t app_credentials_init(void);
esp_err_t app_credentials_load(app_credentials_t *credentials);
esp_err_t app_credentials_state(
    const app_credentials_t *credentials, app_provision_state_t *state);
esp_err_t app_credentials_store_wifi(const char *ssid, const char *password);
esp_err_t app_credentials_load_state(app_provision_state_t *state);
