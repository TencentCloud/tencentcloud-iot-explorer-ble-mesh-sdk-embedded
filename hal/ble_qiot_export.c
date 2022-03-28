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
#include <string.h>
#include "ble_qiot_hmac256.h"
#include "ble_qiot_sha256.h"
#include "ble_qiot_utils_base64.h"
#include "ble_qiot_export.h"
#include "ble_qiot_common.h"
#include "ble_qiot_utils_mesh.h"
#include "ble_qiot_mesh_cfg.h"
#include "ble_qiot_template.h"
#include "ble_qiot_log.h"

llsync_mesh_scan_head_t llsync_mesh_scan_head = {
    .flags             = {0x02, 0x01, 0x06},
    .length            = 0x0d,
    .type              = 0x09,
};

extern ble_device_info_t sg_dev_info;

const char *mesh_hex(const void *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    static char hexbufs[4][137];
    static uint8_t curbuf;
    const uint8_t *b = buf;
    char *str;
    int i;

    str = hexbufs[curbuf++];
    curbuf %= ARRAY_SIZE(hexbufs);

    len = Min(len, (sizeof(hexbufs[0]) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2] = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

// mesh接收数据处理
ble_qiot_ret_status_t llsync_mesh_recv_data_handle(uint32_t Opcode, uint8_t *data, uint16_t data_len)
{
    if (!data || !data_len) {
        ble_qiot_log_e("param err.");
        return LLSYNC_MESH_RS_ERR_PARA;
    }

    switch (Opcode) {
        case LLSYNC_MESH_VND_MODEL_OP_SET: {
            llsync_mesh_vendor_data_set(data, data_len);
            break;
        }

        case LLSYNC_MESH_VND_MODEL_OP_GET: {
            llsync_mesh_vendor_data_get(data, data_len);
            break;
        }

        case LLSYNC_MESH_VND_MODEL_OP_SET_UNACK: {
            llsync_mesh_vendor_data_set_unack(data, data_len);
            break;
        }

        case LLSYNC_MESH_VND_MODEL_OP_CONFIRMATION: {
            llsync_mesh_vendor_op_confirmation(data, data_len);
            break;
        }

        default: 
            break;
    }

    return LLSYNC_MESH_RS_OK;
}

// 扫描响应数据获取 已验证
ble_qiot_ret_status_t llsync_mesh_scan_data_get(uint8_t *data, uint8_t *data_len)
{
    uint8_t index                           = sizeof(llsync_mesh_scan_head_t);
    uint8_t scan_data[BLE_MESH_ADV_MAX_LEN] = {0};

    if (!data) {
        ble_qiot_log_e("param err.");
        return LLSYNC_MESH_RS_ERR_PARA;
    }

    memcpy(scan_data, &llsync_mesh_scan_head, index);
    memcpy(&scan_data[index], sg_dev_info.device_name, strlen(sg_dev_info.device_name));
    index += strlen(sg_dev_info.device_name);;

    memcpy(data, scan_data, index);
    *data_len = index;

    return LLSYNC_MESH_RS_OK;
}

// UUID获取接口 已验证
int llsync_mesh_dev_uuid_get(uint8_t type, uint8_t *data, uint8_t data_len)
{
    uint8_t index = 0;

    if (!data || BLE_MESH_DEVICE_UUID_LEN >= data_len) {
        ble_qiot_log_e("param err.");
        return LLSYNC_MESH_RS_ERR_PARA;
    }

    data[index++] = BLE_MESH_CID_VENDOR_LOW;
    data[index++] = BLE_MESH_CID_VENDOR_HIGH;
    if (type == LLSYNC_MESH_UNNET_ADV_BIT)
        data[index++] = LLSYNC_MESH_SDK_VERSION | LLSYNC_MESH_UNNET_ADV_BIT;
    else
        data[index++] = LLSYNC_MESH_SDK_VERSION | LLSYNC_MESH_SILENCE_ADV_BIT;
    data[index++] = LLSYNC_MESH_ADV_RFU_FLAG2;
    memcpy(&data[index], sg_dev_info.product_id, sizeof(sg_dev_info.product_id));
    index += sizeof(sg_dev_info.product_id);
    data[index] = '\0';

    return index;
}

// AuthValue计算  已验证
ble_qiot_ret_status_t llsync_mesh_auth_clac(uint8_t *random, uint8_t *auth_data)
{
    uint8_t temp_data[BLE_MESH_RAMDOM_DATA_LEN] = {0};
    uint8_t sign[HMAC_SHA256_DIGEST_SIZE]       = {0};
    uint8_t buf[128]                            = {0};
    uint8_t buf_len                             = 0;
    uint8_t i                                   = 0;

    if (auth_data == NULL || random == NULL) {
        ble_qiot_log_i("param err.");
        return LLSYNC_MESH_RS_ERR_PARA;
    }

    memcpy(buf, mesh_hex(random, BLE_MESH_RAMDOM_DATA_LEN), BLE_MESH_RAMDOM_DATA_LEN*2);

    buf_len += BLE_MESH_RAMDOM_DATA_LEN*2;
    memcpy(buf + buf_len, sg_dev_info.product_id, BLE_QIOT_PRODUCT_ID_LEN);
    buf_len += BLE_QIOT_PRODUCT_ID_LEN;
    memcpy(buf + buf_len, sg_dev_info.device_name, strlen(sg_dev_info.device_name));
    buf_len += strlen(sg_dev_info.device_name);

    llsync_hmac_sha256(sign, (const uint8_t *)buf, buf_len,
                    (const uint8_t*)sg_dev_info.psk, sizeof(sg_dev_info.psk));

    for (i = 0; i < HMAC_SHA256_DIGEST_SIZE / 2; i++) {
        temp_data[i] = sign[i] ^ sign[i + HMAC_SHA256_DIGEST_SIZE / 2];
    }

    memcpy(auth_data, temp_data, BLE_MESH_RAMDOM_DATA_LEN);

    return LLSYNC_MESH_RS_OK;
}

#ifdef __cplusplus
}
#endif
