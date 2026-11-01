// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#include "bluetooth_demo.h"
#include "bluetooth.h"

#include <stdbool.h>
#include "esp_log.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"

#define APP_BLUETOOTH_SCAN_MS 10000
#define APP_BLUETOOTH_PAUSE_MS 60000

static const char *TAG = "app_bluetooth_demo";
static uint8_t own_address_type;
static unsigned int report_count;
static bool timer_initialized;
static struct ble_npl_callout scan_timer;

static void schedule_scan(void)
{
    ble_npl_callout_reset(&scan_timer, ble_npl_time_ms_to_ticks32(APP_BLUETOOTH_PAUSE_MS));
}

static int scan_event(struct ble_gap_event *event, void *argument)
{
    (void)argument;
    if (event->type == BLE_GAP_EVENT_DISC) {
        const uint8_t *address = event->disc.addr.val;
        report_count++;
        ESP_LOGI(TAG, "BLE %02x:%02x:%02x:%02x:%02x:%02x RSSI %d dBm",
                 address[5], address[4], address[3], address[2], address[1], address[0],
                 event->disc.rssi);
    } else if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        ESP_LOGI(TAG, "Scan finished: %u advertisement reports (reason %d)",
                 report_count, event->disc_complete.reason);
        schedule_scan();
    }
    return 0;
}

static void scan(struct ble_npl_event *event)
{
    (void)event;
    const struct ble_gap_disc_params parameters = {
        .passive = 1,
        .itvl = 160,
        .window = 80,
        .filter_duplicates = 1,
    };
    report_count = 0;
    int result = ble_gap_disc(own_address_type, APP_BLUETOOTH_SCAN_MS,
                             &parameters, scan_event, NULL);
    if (result != 0) {
        ESP_LOGW(TAG, "Scan failed (%d); retrying in 60 seconds", result);
        schedule_scan();
    }
}

static void host_ready(uint8_t address_type)
{
    own_address_type = address_type;
    if (!timer_initialized) {
        ble_npl_callout_init(&scan_timer, nimble_port_get_dflt_eventq(), scan, NULL);
        timer_initialized = true;
    }
    scan(NULL);
}

static void host_reset(int reason)
{
    (void)reason;
    if (timer_initialized) {
        ble_npl_callout_stop(&scan_timer);
    }
}

esp_err_t app_bluetooth_demo_start(void)
{
    const app_bluetooth_callbacks_t callbacks = {
        .ready = host_ready,
        .reset = host_reset,
    };
    return app_bluetooth_start(&callbacks);
}
