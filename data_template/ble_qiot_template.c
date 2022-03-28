/*
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "ble_qiot_template.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "ble_qiot_common.h"
#include "ble_qiot_log.h"

uint8_t onoff = 0;
uint8_t unix_time[6] = {0};
uint8_t power_percent = 0;
uint16_t HSL[3] = {0};
uint16_t lightness_level = 0;
uint16_t color_temperature = 0;

static int llsync_mesh_onoff_set(const char *data, uint16_t len)
{
    onoff = data[0];
    memcpy(&onoff, data, len);
    ble_qiot_log_i("Mesh set data is OnOff:%d", onoff);
    return 0;
}

static int llsync_mesh_onoff_get(char *data, uint16_t len)
{
	memcpy(data, &onoff, sizeof(onoff));
    return sizeof(onoff);
}

static int llsync_mesh_unix_time_set(const char *data, uint16_t len)
{
    uint8_t i = 0;

    memcpy(unix_time, data, len);

    for (i=0; i < 6; i++) {
        ble_qiot_log_i("Mesh set data is unix_time:%d-0x%02x", i, unix_time[i]);
    }
    return 0;
}

static int llsync_mesh_unix_time_get(char *data, uint16_t len)
{
    memcpy(data, unix_time, sizeof(unix_time));
    return sizeof(unix_time);
}

static int llsync_mesh_power_percent_set(const char *data, uint16_t len)
{
    memcpy(&power_percent, data, len);
    return 0;
}

static int llsync_mesh_power_percent_get(char *data, uint16_t len)
{
    memcpy(data, &power_percent, sizeof(power_percent));
    return sizeof(power_percent);
}

static int llsync_mesh_HSL_set(const char *data, uint16_t len)
{
    memcpy(HSL, data, len);
    return 0;
}

static int llsync_mesh_HSL_get(char *data, uint16_t len)
{
    memcpy(data, HSL, sizeof(HSL));
    return sizeof(HSL);
}

static int llsync_mesh_lightness_level_set(const char *data, uint16_t len)
{
    memcpy(&lightness_level, data, len);
    return 0;
}

static int llsync_mesh_lightness_level_get(char *data, uint16_t len)
{
    memcpy(data, &lightness_level, sizeof(lightness_level));
    return sizeof(lightness_level);
}

static int llsync_mesh_color_temperature_set(const char *data, uint16_t len)
{
    memcpy(&color_temperature, data, len);
    return 0;
}

static int llsync_mesh_color_temperature_get(char *data, uint16_t len)
{
    memcpy(data, &color_temperature, sizeof(color_temperature));
    return sizeof(color_temperature);
}

ble_property_t sg_ble_property_array[6] = {
	{llsync_mesh_onoff_set, llsync_mesh_onoff_get, LLSYNC_MESH_VENDOR_ONOFF_TYPE, sizeof(llsync_mesh_onoff)},
	{llsync_mesh_unix_time_set, llsync_mesh_unix_time_get, LLSYNC_MESH_VENDOR_UNIX_TIME_TYPE, sizeof(llsync_mesh_unix_time)},
    {llsync_mesh_power_percent_set, llsync_mesh_power_percent_get, LLSYNC_MESH_VENDOR_POWER_PERCENT_TYPE, sizeof(llsync_mesh_power_percent)},
    {llsync_mesh_HSL_set, llsync_mesh_HSL_get, LLSYNC_MESH_VENDOR_HSL_TYPE, sizeof(llsync_mesh_HSL)},
    {llsync_mesh_lightness_level_set, llsync_mesh_lightness_level_get, LLSYNC_MESH_VENDOR_LIGHTNESS_LEVEL_TYPE, sizeof(llsync_mesh_lightness_level)},
    {llsync_mesh_color_temperature_set, llsync_mesh_color_temperature_get, LLYNSC_MESH_VENDOR_COLOR_TEMPRETURE_TYPE, sizeof(llsync_mesh_color_temperature)},
};

uint8_t sg_llsync_vendor_size = sizeof(sg_ble_property_array)/sizeof(sg_ble_property_array[0]);

#ifdef __cplusplus
}
#endif
