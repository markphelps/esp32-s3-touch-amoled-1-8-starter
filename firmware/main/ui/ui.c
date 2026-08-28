// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#include "ui.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "bsp/display.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include "wifi.h"

#define UI_UPDATE_INTERVAL_MS 250
#define BRIGHTNESS_HIGH 85
#define BRIGHTNESS_LOW 40

static const char *TAG = "app_ui";
static lv_obj_t *wifi_label;
static lv_obj_t *heap_label;
static uint8_t brightness = BRIGHTNESS_HIGH;
// Core 0 publishes readiness without making an LVGL call outside the LVGL task.
static atomic_bool online;

static void update_status(lv_timer_t *timer)
{
    (void)timer;
    app_wifi_status_t wifi_status = {0};
    app_wifi_get_status(&wifi_status);
    const bool notified_online = atomic_load_explicit(&online, memory_order_acquire);
    const char *state;
    if (wifi_status.connected) {
        state = notified_online ? "online" : "connecting";
    } else {
        state = notified_online ? "reconnecting" : "offline";
    }

    // LVGL invokes timers in its task, so this callback must not take the display lock.
    lv_label_set_text_fmt(wifi_label, "WiFi: %s", state);
    lv_label_set_text_fmt(heap_label, "Free heap: %u bytes", (unsigned)esp_get_free_heap_size());
}

static void screen_clicked(lv_event_t *event)
{
    (void)event;
    const uint8_t next_brightness =
        brightness == BRIGHTNESS_HIGH ? BRIGHTNESS_LOW : BRIGHTNESS_HIGH;
    esp_err_t error = bsp_display_brightness_set(next_brightness);
    if (error == ESP_OK) {
        brightness = next_brightness;
    } else {
        ESP_LOGW(TAG, "Brightness change failed: %s", esp_err_to_name(error));
    }
}

static void configure_label(lv_obj_t *label)
{
    lv_obj_set_width(label, 336);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);
}

esp_err_t app_ui_start(void)
{
    // The caller holds the LVGL lock while this function builds the screen.
    lv_obj_t *screen = lv_screen_active();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x080808), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, screen_clicked, LV_EVENT_CLICKED, NULL);

    wifi_label = lv_label_create(screen);
    heap_label = lv_label_create(screen);
    if (wifi_label == NULL || heap_label == NULL) {
        return ESP_ERR_NO_MEM;
    }
    configure_label(wifi_label);
    configure_label(heap_label);
    lv_obj_align(wifi_label, LV_ALIGN_CENTER, 0, -30);
    lv_obj_align(heap_label, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(wifi_label, "WiFi: offline");
    lv_label_set_text(heap_label, "Free heap: --");

    if (lv_timer_create(update_status, UI_UPDATE_INTERVAL_MS, NULL) == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void app_ui_notify_online(void)
{
    atomic_store_explicit(&online, true, memory_order_release);
}
