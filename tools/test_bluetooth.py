#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Mark Phelps
# SPDX-License-Identifier: Apache-2.0

"""Exercise the firmware Bluetooth lifecycle with a deterministic host adapter."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile

FAKE_HEADER = r"""
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_INVALID_STATE 2
void fake_log(const char *tag, const char *format, ...);
#define ESP_LOGI(...) fake_log(__VA_ARGS__)
#define ESP_LOGW(...) fake_log(__VA_ARGS__)
#define ESP_LOGE(...) fake_log(__VA_ARGS__)
extern struct fake_host_configuration {
    void (*sync_cb)(void);
    void (*reset_cb)(int);
} ble_hs_cfg;
int ble_hs_util_ensure_addr(int prefer_random);
int ble_hs_id_infer_auto(int privacy, uint8_t *address_type);
esp_err_t nimble_port_init(void);
void nimble_port_run(void);
void nimble_port_freertos_init(void (*task)(void *));
void nimble_port_freertos_deinit(void);
"""

TEST_SOURCE = r"""
#include <assert.h>
#include "bluetooth.h"
#include "fake.h"

struct fake_host_configuration ble_hs_cfg;
static int initialization_result = 42;
static int address_result;
static int inference_result;
static int initialization_count;
static int ready_count;
static int reset_reason;
static int run_count;
static int deinit_count;
static void (*host_task)(void *);
void fake_log(const char *tag, const char *format, ...) { (void)tag; (void)format; }
int ble_hs_util_ensure_addr(int prefer_random) { assert(!prefer_random); return address_result; }
int ble_hs_id_infer_auto(int privacy, uint8_t *address_type) {
    assert(!privacy); *address_type = 7; return inference_result;
}
esp_err_t nimble_port_init(void) { initialization_count++; return initialization_result; }
void nimble_port_run(void) { run_count++; }
void nimble_port_freertos_init(void (*task)(void *)) { assert(host_task == NULL); host_task = task; }
void nimble_port_freertos_deinit(void) { deinit_count++; }
static void ready(uint8_t address_type) { assert(address_type == 7); ready_count++; }
static void reset(int reason) { reset_reason = reason; }

int main(void)
{
    app_bluetooth_callbacks_t callbacks = {0};
    assert(app_bluetooth_start(NULL) == ESP_ERR_INVALID_ARG);
    assert(app_bluetooth_start(&callbacks) == ESP_ERR_INVALID_ARG);
    assert(initialization_count == 0);
    callbacks.ready = ready;
    callbacks.reset = reset;
    assert(app_bluetooth_start(&callbacks) == 42);
    assert(host_task == NULL);
    initialization_result = ESP_OK;
    assert(app_bluetooth_start(&callbacks) == ESP_OK);
    assert(initialization_count == 2);
    assert(host_task != NULL);
    assert(app_bluetooth_start(&callbacks) == ESP_ERR_INVALID_STATE);
    assert(initialization_count == 2);
    // The caller can release or change its configuration after start.
    callbacks.ready = NULL;
    callbacks.reset = NULL;
    address_result = 3;
    ble_hs_cfg.sync_cb();
    assert(ready_count == 0);
    address_result = 0;
    inference_result = 4;
    ble_hs_cfg.sync_cb();
    assert(ready_count == 0);
    inference_result = 0;
    ble_hs_cfg.sync_cb();
    assert(ready_count == 1);
    ble_hs_cfg.reset_cb(19);
    assert(reset_reason == 19);
    ble_hs_cfg.sync_cb();
    assert(ready_count == 2);
    host_task(NULL);
    assert(run_count == 1 && deinit_count == 1);
    return 0;
}
"""


def main() -> None:
    compiler = shutil.which("cc")
    if compiler is None:
        raise SystemExit("C compiler unavailable. Install Xcode command-line tools or GCC.")
    firmware = Path(__file__).resolve().parents[1] / "firmware/main"
    with tempfile.TemporaryDirectory(prefix="app-bluetooth-test-") as directory:
        root = Path(directory)
        (root / "fake.h").write_text(FAKE_HEADER, encoding="utf-8")
        for header in (
            "esp_err.h",
            "esp_log.h",
            "host/ble_hs.h",
            "host/util/util.h",
            "nimble/nimble_port.h",
            "nimble/nimble_port_freertos.h",
        ):
            path = root / header
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "fake.h"\n', encoding="utf-8")
        source = root / "test.c"
        executable = root / "test"
        source.write_text(TEST_SOURCE, encoding="utf-8")
        subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsanitize=address,undefined",
                "-I",
                str(root),
                "-I",
                str(firmware),
                str(source),
                str(firmware / "bluetooth.c"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)
    print("Bluetooth lifecycle checks passed.")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        raise SystemExit(130) from None
