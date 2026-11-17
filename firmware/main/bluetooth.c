// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#include "bluetooth.h"

#include <stdbool.h>
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "app_bluetooth";
static bool started;
static app_bluetooth_callbacks_t client;

static void host_reset(int reason)
{
    ESP_LOGW(TAG, "Bluetooth host reset: %d", reason);
    if (client.reset != NULL) {
        client.reset(reason);
    }
}

static void host_synced(void)
{
    uint8_t own_address_type;
    int result = ble_hs_util_ensure_addr(0);
    if (result == 0) {
        result = ble_hs_id_infer_auto(0, &own_address_type);
    }
    if (result != 0) {
        ESP_LOGE(TAG, "Bluetooth address unavailable (%d); restart the board", result);
        return;
    }
    ESP_LOGI(TAG, "Bluetooth ready");
    client.ready(own_address_type);
}

static void host_task(void *argument)
{
    (void)argument;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t app_bluetooth_start(const app_bluetooth_callbacks_t *callbacks)
{
    if (callbacks == NULL || callbacks->ready == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t error = nimble_port_init();
    if (error != ESP_OK) {
        return error;
    }
    client = *callbacks;
    ble_hs_cfg.sync_cb = host_synced;
    ble_hs_cfg.reset_cb = host_reset;
    started = true;
    nimble_port_freertos_init(host_task);
    return ESP_OK;
}
