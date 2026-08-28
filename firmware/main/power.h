// SPDX-FileCopyrightText: 2026 Mark Phelps
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t app_power_start(void);
bool app_power_on_battery(void);
