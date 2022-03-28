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
#include <stdio.h>
#include "ble_qiot_utils_mesh.h"
#include "ble_qiot_common.h"
#include "ble_qiot_mesh_cfg.h"
#include "ble_qiot_template.h"
#include "ble_qiot_export.h"
#include "ble_qiot_import.h"
#include "ble_qiot_log.h"
#include "ble_qiot_hmac256.h"
#include "ble_qiot_utils_base64.h"

extern ble_property_t sg_ble_property_array[];
extern uint8_t sg_llsync_vendor_size;

ble_device_info_t sg_dev_info;
static ble_core_data sg_core_data;

static llsync_mesh_distribution_net_t sg_mesh_net;

llsync_mesh_unnet_head_t llsync_mesh_common_head = {
    .flags             = {0x02, 0x01, 0x06},
    .service_uuid_list = {0x03, 0x03, 0x18, 0x27},
    .length            = 0x15,
    .type              = 0x16,
    .mesh_seivice      = 0x1827,
};

static ble_mesh_handle_t sg_mesh_handle = {
    .reoport_tid_num = BLE_MESH_REPORT_MIN_TID,
    .recv_tid_num    = LLSYNC_MESH_TID_RESET_VALUE,
};

static int llsync_mesh_data_bind_set(uint8_t *recv_data, uint8_t *resp_data)
{
    char    sign_info[100]                   = {0};
    uint8_t sign[HMAC_SHA256_DIGEST_SIZE]    = {0};
    uint8_t sign_info_len                    = 0;
    int     time_expiration                  = 0;
    int     nonce                            = 0;
    uint8_t secret[BLE_QIOT_PSK_LEN / 4 * 3] = {0};
    size_t  secret_len                       = 0;
    uint8_t index                            = 0;
    int     i                                = 0;

    llsync_mesh_bind_data bind_data_aligned;
    memcpy(&bind_data_aligned, recv_data, sizeof(llsync_mesh_bind_data));

    nonce           = NTOHL(bind_data_aligned.nonce);
    time_expiration = BLE_GET_EXPIRATION_TIME(NTOHL(bind_data_aligned.timestamp));

    // [10 bytes product_id] + [x bytes device name] + ';' + [4 bytes nonce] + ';' + [4 bytes timestamp]
    memcpy(sign_info, sg_dev_info.product_id, sizeof(sg_dev_info.product_id));
    sign_info_len += sizeof(sg_dev_info.product_id);
    memcpy(sign_info + sign_info_len, sg_dev_info.device_name, strlen(sg_dev_info.device_name));
    sign_info_len += strlen(sg_dev_info.device_name);
    snprintf(sign_info + sign_info_len, sizeof(sign_info) - sign_info_len, ";%u", nonce);
    sign_info_len += strlen(sign_info + sign_info_len);
    snprintf(sign_info + sign_info_len, sizeof(sign_info) - sign_info_len, ";%u", time_expiration);
    sign_info_len += strlen(sign_info + sign_info_len);

    ble_qiot_log_i("------------------product_id: %s", sg_dev_info.product_id);
    ble_qiot_log_i("------------------device_name: %s", sg_dev_info.device_name);
    ble_qiot_log_i("------------------nonce: %u", nonce);
    ble_qiot_log_i("------------------time_expiration: %u", time_expiration);

    for (i = 0; i < sign_info_len; i++) {
        ble_qiot_log_i("hmac before--: %d-%x", i, sign_info[i]);
    }

    qcloud_iot_utils_base64decode(secret, sizeof(secret), (size_t *)&secret_len,
                                  (const unsigned char *)sg_dev_info.psk, sizeof(sg_dev_info.psk));

    llsync_hmac_sha256(sign, (const uint8_t *)sign_info, sign_info_len, (const uint8_t*)secret, secret_len);

    ble_qiot_log_i("************************************************************");

    for (i = 0; i < HMAC_SHA256_DIGEST_SIZE; i++) {
        ble_qiot_log_i("vendor bind sign--: %d-%x", i, sign[i]);
    }

    resp_data[index++] = LLSYNC_MESH_ATT_BIND_RESP_LOW;
    resp_data[index++] = LLSYNC_MESH_ATT_BIND_RESP_HIGH;
    memcpy(&resp_data[index], sign, HMAC_SHA256_DIGEST_SIZE);
    index += HMAC_SHA256_DIGEST_SIZE;

    return index;
}

void llsync_mesh_vendor_op_confirmation(uint8_t *recv_data, uint16_t data_len)
{
    if (recv_data[LLSYNC_MESH_VENDOR_TID_INDEX] == sg_mesh_handle.reoport_tid_num) {
        ble_qiot_log_i("The indication report success!");
        sg_mesh_handle.reoport_tid_num = (BLE_MESH_REPORT_MAX_TID != sg_mesh_handle.reoport_tid_num) ? (sg_mesh_handle.reoport_tid_num+1) : BLE_MESH_REPORT_MIN_TID;
    }
}

static uint8_t llsync_mesh_vendor_recv_tid_handle(uint8_t tid, uint8_t *resp_buf)
{
    uint8_t ret = 0;

    ble_qiot_log_i("sg_mesh_handle.recv_tid_num:%d, tid:%d", sg_mesh_handle.recv_tid_num, tid);

    if (tid > BLE_MESH_RECV_MAX_TID) {
        ble_qiot_log_e("Recv tid num err:%d", tid);
        return LLSYNC_MESH_TID_ERR;
    }

    if ((sg_mesh_handle.recv_tid_num == tid)) {
        ble_qiot_log_i("It is a repeat message.");
        ret = LLSYNC_MESH_TID_REPEAT;
    } else {
        sg_mesh_handle.recv_tid_num = tid;
        sg_mesh_net.timer_type = LLSYNC_MESH_RECV_TID_TIMER;
        llsync_mesh_timer_start(sg_mesh_net.mesh_timer, BLE_MESH_TIMER_PERIOD);
        ret = LLSYNC_MESH_TID_OK;
    }

    if (resp_buf) 
        resp_buf[LLSYNC_MESH_VENDOR_TID_INDEX] = sg_mesh_handle.recv_tid_num;

    return ret;
}

static int llsync_mesh_data_id_get(uint16_t att_type)
{
    uint16_t id = 0;

    for (id = 0; id < sg_llsync_vendor_size; id++) {
        if (att_type == sg_ble_property_array[id].type) {
            ble_qiot_log_d("find id is:%d", id);
            return id;
        }
    }

    ble_qiot_log_e("Do not find this type:0x%02x", att_type);

    return LLSYNC_MESH_RS_ERR;
}

static int ble_user_property_get_data_by_id(uint8_t id, char *buf, uint16_t buf_len)
{
    int ret_len = 0;

    if (NULL != sg_ble_property_array[id].get_cb) {
        if (sg_ble_property_array[id].data_len > buf_len) {
            ble_qiot_log_e("not enough space get property id %d data", id);
            return -1;
        }
        ret_len = sg_ble_property_array[id].get_cb(buf, buf_len);
        if (ret_len < 0) {
            ble_qiot_log_e("get property id %d data failed", id);
            return -1;
        } else {
            if (sg_ble_property_array[id].data_len == ret_len) {
                return ret_len;
            } else {
                ble_qiot_log_e("property id %d length invalid", id);
                return -1;
            }
        }
    }
    ble_qiot_log_e("invalid callback, property id %d", id);

    return 0;
}

void llsync_mesh_vendor_data_set(uint8_t *recv_data, uint16_t data_len)
{
    uint16_t att_type                             = 0;
    uint16_t recv_buf_index                       = LLSYNC_MESH_VENDOR_ATT_TYPE_INDEX;
    uint8_t  resp_buf[BLE_MESH_RESP_DATA_MAX_LEN] = {0};
    uint8_t  resp_buf_index                       = LLSYNC_MESH_VENDOR_ATT_TYPE_INDEX;
    int      att_type_id                          = 0;
    int      ret                                  = 0;

    uint8_t i=0;

    ret = llsync_mesh_vendor_recv_tid_handle(recv_data[LLSYNC_MESH_VENDOR_TID_INDEX], resp_buf);
    if (LLSYNC_MESH_TID_ERR == ret) {
        return;
    } else if (LLSYNC_MESH_TID_REPEAT == ret) {
        goto exit;
    }

    for (i=0; i< data_len; i++) {
        ble_qiot_log_i("recv data is:%d-0x%02x", i, recv_data[i]);
    }

    while (data_len > recv_buf_index) {
        memcpy(&att_type, &recv_data[recv_buf_index], sizeof(att_type));
        recv_buf_index += sizeof(att_type);

        ble_qiot_log_i("Att type is:0x%02x", att_type);
        if ((LLSYNC_MESH_BIND_OK != sg_core_data.bind_state) && (LLSYNC_MESH_ATT_BIND_TYPE == att_type)) {
            recv_buf_index += sizeof(llsync_mesh_bind_data);
            resp_buf_index += llsync_mesh_data_bind_set(recv_data, resp_buf);
            break;
        }

        att_type_id = llsync_mesh_data_id_get(att_type);
        if (LLSYNC_MESH_RS_ERR == att_type_id) {
            ble_qiot_log_e("The att type err.");
            return;
        }

        if (NULL != sg_ble_property_array[att_type_id].set_cb) {
            if (0 != sg_ble_property_array[att_type_id].set_cb((char *)&recv_data[recv_buf_index], sg_ble_property_array[att_type_id].data_len)) {
                ble_qiot_log_e("set property id %d failed", att_type_id);
                return ;
            }
        }
        recv_buf_index += sg_ble_property_array[att_type_id].data_len;

        // response
        memcpy(&resp_buf[resp_buf_index], &att_type, sizeof(att_type));
        resp_buf_index += sizeof(att_type);

        ret = ble_user_property_get_data_by_id(att_type_id, (char *)&resp_buf[resp_buf_index],
                                                        sizeof(resp_buf) - resp_buf_index);
        if (ret < 0) {
            ble_qiot_log_e("get property id %d failed", att_type_id);
            return ;
        } else {
            resp_buf_index += ret;
        }
    }

exit:
    llsync_mesh_vendor_data_send(LLSYNC_MESH_VND_MODEL_OP_STATUS, resp_buf, resp_buf_index);
}

void llsync_mesh_vendor_data_set_unack(uint8_t *recv_data, uint16_t data_len)
{
    uint16_t att_type       = 0;
    uint16_t recv_buf_index = LLSYNC_MESH_VENDOR_ATT_TYPE_INDEX;
    int      att_type_id    = 0;

    if (LLSYNC_MESH_TID_OK != llsync_mesh_vendor_recv_tid_handle(recv_data[LLSYNC_MESH_VENDOR_TID_INDEX], NULL)) {
        return;
    }

    while (data_len > recv_buf_index) {
        memcpy(&att_type, &recv_data[recv_buf_index], sizeof(att_type));
        recv_buf_index += sizeof(att_type);

        ble_qiot_log_i("Att type is:0x%02x", att_type);

        att_type_id = llsync_mesh_data_id_get(att_type);
        if (LLSYNC_MESH_RS_ERR == att_type_id) {
            ble_qiot_log_e("The att type err.");
            return;
        }

        if (NULL != sg_ble_property_array[att_type_id].set_cb) {
            if (0 != sg_ble_property_array[att_type_id].set_cb((char *)&recv_data[recv_buf_index], sg_ble_property_array[att_type_id].data_len)) {
                ble_qiot_log_e("set property id %d failed", att_type_id);
                return;
            }
        }
        recv_buf_index += sg_ble_property_array[att_type_id].data_len;
    }
}

void llsync_mesh_vendor_data_get(uint8_t *recv_data, uint16_t data_len)
{
    uint16_t att_type                             = 0;
    uint16_t recv_buf_index                       = LLSYNC_MESH_VENDOR_ATT_TYPE_INDEX;
    uint8_t  resp_buf[BLE_MESH_RESP_DATA_MAX_LEN] = {0};
    uint8_t  resp_buf_index                       = LLSYNC_MESH_VENDOR_ATT_TYPE_INDEX;
    int      ret                                  = 0;
    int      att_type_id                          = 0;

    ret = llsync_mesh_vendor_recv_tid_handle(recv_data[LLSYNC_MESH_VENDOR_TID_INDEX], resp_buf);
    if (LLSYNC_MESH_TID_ERR == ret) {
        return;
    } else if (LLSYNC_MESH_TID_REPEAT == ret) {
        goto exit;
    }

    while (data_len > recv_buf_index) {
        memcpy(&att_type, &recv_data[recv_buf_index], sizeof(att_type));
        recv_buf_index += sizeof(att_type);

        att_type_id = llsync_mesh_data_id_get(att_type);
        if (LLSYNC_MESH_RS_ERR == att_type_id) {
            ble_qiot_log_e("The att type err.");
            return;
        }

        ble_qiot_log_i("Att type is:0x%02x", att_type);

        // response
        memcpy(&resp_buf[resp_buf_index], &att_type, sizeof(att_type));
        resp_buf_index += sizeof(att_type);

        ret = ble_user_property_get_data_by_id(att_type_id, (char *)&resp_buf[resp_buf_index],
                                                        sizeof(resp_buf) - resp_buf_index);
        if (ret < 0) {
            ble_qiot_log_e("get property id %d failed", att_type_id);
            return;
        } else {
            resp_buf_index += ret;
        }
    }

exit:
    llsync_mesh_vendor_data_send(LLSYNC_MESH_VND_MODEL_OP_STATUS, resp_buf, resp_buf_index);
}

static void llsync_mesh_unnet_adv_start(void)
{
    uint8_t index = sizeof(llsync_mesh_unnet_head_t);

    memcpy(sg_mesh_net.adv, &llsync_mesh_common_head, index);
    index += llsync_mesh_dev_uuid_get(LLSYNC_MESH_UNNET_ADV, &sg_mesh_net.adv[index], BLE_MESH_ADV_MAX_LEN-index);
    llsync_mesh_adv_handle(LLSYNC_MESH_ADV_START, sg_mesh_net.adv, index);
    sg_mesh_net.adv_status = LLSYNC_MESH_ADV_START;
    sg_mesh_net.data_len = index;
    sg_mesh_net.timer_type = LLSYNC_MESH_UNNET_ADV_TIMER;
    llsync_mesh_timer_start(sg_mesh_net.mesh_timer, BLE_MESH_TIMER_PERIOD);
}

static void llsync_mesh_silence_adv_start(void)
{
    uint8_t index = sizeof(llsync_mesh_unnet_head_t);

    memcpy(sg_mesh_net.adv, &llsync_mesh_common_head, index);
    index += llsync_mesh_dev_uuid_get(LLSYNC_MESH_SILENCE_ADV, &sg_mesh_net.adv[index], BLE_MESH_ADV_MAX_LEN-index);
    llsync_mesh_adv_handle(LLSYNC_MESH_ADV_START, sg_mesh_net.adv, index);
    sg_mesh_net.adv_status = LLSYNC_MESH_ADV_START;
    sg_mesh_net.data_len = index;
    sg_mesh_net.timer_type = LLSYNC_MESH_SILENCE_ADV_TIMER;
    llsync_mesh_timer_start(sg_mesh_net.mesh_timer, BLE_MESH_TIMER_PERIOD);
}

void llsync_mesh_timer_cb(void *param)
{
    sg_mesh_net.timer_cnt++;

    switch (sg_mesh_net.timer_type) {
        case LLSYNC_MESH_UNNET_ADV_TIMER: {
            if ((LLSYNC_MESH_ADV_START == sg_mesh_net.adv_status) && (sg_mesh_net.timer_cnt >= ((int)BLE_MESH_UNNET_ADV_DURATION/BLE_MESH_TIMER_PERIOD))) {
                sg_mesh_net.timer_total_cnt += sg_mesh_net.timer_cnt;
                sg_mesh_net.timer_cnt = 0;
                llsync_mesh_adv_handle(LLSYNC_MESH_ADV_STOP, NULL, 0);
                sg_mesh_net.adv_status = LLSYNC_MESH_ADV_STOP;
            }

            if ((LLSYNC_MESH_ADV_STOP == sg_mesh_net.adv_status) && (sg_mesh_net.timer_cnt >= ((int)BLE_MESH_UNNET_ADV_INTERVAL/BLE_MESH_TIMER_PERIOD))) {
                sg_mesh_net.timer_total_cnt += sg_mesh_net.timer_cnt;
                sg_mesh_net.timer_cnt = 0;
                llsync_mesh_adv_handle(LLSYNC_MESH_ADV_START, sg_mesh_net.adv, sg_mesh_net.data_len);
                sg_mesh_net.adv_status = LLSYNC_MESH_ADV_START;
            }

            if ((sg_mesh_net.timer_total_cnt*BLE_MESH_TIMER_PERIOD) >= BLE_MESH_UNNET_ADV_TOTAL_TIME) {
                ble_qiot_log_i("Unallocated network timeout time is up.");
                sg_mesh_net.timer_total_cnt = 0;
                sg_mesh_net.timer_cnt = 0;

                if (BLE_MESH_SILENCE_ADV_ENABLE) {
                    llsync_mesh_silence_adv_start();
                } else {
                    llsync_mesh_adv_handle(LLSYNC_MESH_ADV_STOP, NULL, 0);
                    sg_mesh_net.adv_status = LLSYNC_MESH_ADV_STOP;
                    llsync_mesh_timer_stop(sg_mesh_net.mesh_timer);
                }
            }
            break;
        }

        case LLSYNC_MESH_SILENCE_ADV_TIMER: {
            if ((LLSYNC_MESH_ADV_START == sg_mesh_net.adv_status) && (sg_mesh_net.timer_cnt >= ((int)BLE_MESH_SILENCE_ADV_DURATION/BLE_MESH_TIMER_PERIOD))) {
                sg_mesh_net.timer_cnt = 0;
                llsync_mesh_adv_handle(LLSYNC_MESH_ADV_STOP, NULL, 0);
                sg_mesh_net.adv_status = LLSYNC_MESH_ADV_STOP;
            }

            if ((LLSYNC_MESH_ADV_STOP == sg_mesh_net.adv_status) && (sg_mesh_net.timer_cnt >= ((int)BLE_MESH_SILENCE_ADV_INTERVAL/BLE_MESH_TIMER_PERIOD))) {
                sg_mesh_net.timer_cnt = 0;
                llsync_mesh_adv_handle(LLSYNC_MESH_ADV_START, sg_mesh_net.adv, sg_mesh_net.data_len);
                sg_mesh_net.adv_status = LLSYNC_MESH_ADV_START;
            }
            break;
        }

        case LLSYNC_MESH_RECV_TID_TIMER:
            if (sg_mesh_net.timer_cnt >= ((int)LLSYNC_MESH_TID_UPDATE_PERIOD/BLE_MESH_TIMER_PERIOD)) {
                sg_mesh_net.timer_cnt = 0;
                sg_mesh_handle.recv_tid_num = LLSYNC_MESH_TID_RESET_VALUE;
                llsync_mesh_timer_stop(sg_mesh_net.mesh_timer);
            }
            break;

        default: 
            break;
    }
    
}

// 主动数据上报
void llsync_mesh_vendor_data_report(void)
{
    int ret = 0;
    uint8_t resp_buf[BLE_MESH_RESP_DATA_MAX_LEN] = {0};
    uint8_t resp_buf_index = 0;
    uint16_t id = 0;
    uint16_t att_type = 0;

    resp_buf[resp_buf_index++] = sg_mesh_handle.reoport_tid_num;

    for (id = 0; id < sg_llsync_vendor_size; id++) {
        memcpy(&att_type, &sg_ble_property_array[id].type, sizeof(att_type));

        ble_qiot_log_i("Att type is:0x%02x", att_type);

        // response
        memcpy(&resp_buf[resp_buf_index], &att_type, sizeof(att_type));
        resp_buf_index += sizeof(att_type);

        ret = ble_user_property_get_data_by_id(id, (char *)&resp_buf[resp_buf_index],
                                                        sizeof(resp_buf) - resp_buf_index);
        if (ret < 0) {
            ble_qiot_log_e("get property id %d failed", id);
            return;
        } else {
            resp_buf_index += ret;
        }
    }

    llsync_mesh_vendor_data_send(LLSYNC_MESH_VND_MODEL_OP_INDICATION, resp_buf, resp_buf_index);

}

// 设备初始化接口
void llsync_mesh_init(void)
{
    llsync_mesh_dev_info_get(&sg_dev_info);

    if (BLE_MESH_DEV_NAME_LEN != strlen(sg_dev_info.device_name)) {
        ble_qiot_log_e("dev name err.");
        return;
    }

    // if (sizeof(sg_core_data) !=
    //     llsync_mesh_flash_handle(LLSYNC_MESH_READ_FLASH, LLSYNC_MESH_RECORD_FLASH_ADDR, (char *)&sg_core_data, sizeof(sg_core_data))) {
    //     ble_qiot_log_e("llsync read flash failed");
    //     return;
    // }

    sg_mesh_net.mesh_timer = llsync_mesh_timer_create(LLSYNC_MESH_TIMER_PERIOD_TYPE, llsync_mesh_timer_cb);
    if (NULL == sg_mesh_net.mesh_timer) {
        ble_qiot_log_e("llsync adv timer create failed");
        return;
    }

    if (LLSYNC_MESH_BIND_OK != sg_core_data.bind_state) {
        // llsync_mesh_unnet_adv_start();
    }

    return;
}

#ifdef __cplusplus
}
#endif
