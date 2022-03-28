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
#ifndef QCLOUD_BLE_QIOT_MESH_CFG_H
#define QCLOUD_BLE_QIOT_MESH_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define LLSYNC_MESH_SDK_VERSION             (1)

#define LLSYNC_MESH_RECORD_FLASH_ADDR       0xFE000  // qiot data storage address
#define LLSYNC_MESH_RECORD_FLASH_PAGESIZE   4096     // flash page size, see chip datasheet

#define LLSYNC_MESH_UNNET_ADV_BIT           (0 << 3)

#define LLSYNC_MESH_SILENCE_ADV_BIT         (1 << 3)

#define LLSYNC_MESH_ADV_RFU_FLAG2           (0x00)

// 未配网广播总的广播时长(ms)
#define BLE_MESH_UNNET_ADV_TOTAL_TIME       (10 * 60 * 1000)

// 未配网超时时间到后仍未配网，是否进入静默广播状态，只用于让Provisioner发现设备
#define BLE_MESH_SILENCE_ADV_ENABLE         (1)

// 未配网广播单次广播时长(ms)
#define BLE_MESH_UNNET_ADV_DURATION         (150)

// 静默广播单次广播时长(ms)
#define BLE_MESH_SILENCE_ADV_DURATION       (150)

// 未配网广播间隔(ms)
#define BLE_MESH_UNNET_ADV_INTERVAL         (500)

// 静默广播间隔(ms)
#define BLE_MESH_SILENCE_ADV_INTERVAL       (60 * 1000)

#define MESH_LOG_PRINT(...)                 printf(__VA_ARGS__)

#define BLE_QIOT_USER_DEFINE_HEXDUMP        (0)

#ifdef __cplusplus
}
#endif

#endif  // QCLOUD_BLE_QIOT_MESH_CFG_H
