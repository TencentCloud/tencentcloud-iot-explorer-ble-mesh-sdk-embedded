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
#ifndef QCLOUD_BLE_QIOT_EXPORT_H
#define QCLOUD_BLE_QIOT_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_qiot_utils_mesh.h"
#include "ble_qiot_common.h"

ble_qiot_ret_status_t llsync_mesh_recv_data_handle(uint32_t Opcode, uint8_t *data, uint16_t data_len);

ble_qiot_ret_status_t llsync_mesh_scan_data_get(uint8_t *data, uint8_t *data_len);

int llsync_mesh_dev_uuid_get(uint8_t type, uint8_t *data, uint8_t data_len);

ble_qiot_ret_status_t llsync_mesh_auth_clac(uint8_t *random, uint8_t *auth_data);

void llsync_mesh_vendor_data_report(void);

#ifdef __cplusplus
}
#endif

#endif  // QCLOUD_BLE_QIOT_EXPORT_H
