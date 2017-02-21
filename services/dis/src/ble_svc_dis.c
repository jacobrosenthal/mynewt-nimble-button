/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sysinit/sysinit.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "config/config.h"
#include "dis/ble_svc_dis.h"

static int
gatt_svr_chr_access_dis(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);


static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: dis */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_SYS_ID_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_MODEL_NUM_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_SERIAL_NUM_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_FW_REV_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_HW_REV_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_SW_REV_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: Read */
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_MFG_NAME_UUID16),
            .access_cb = gatt_svr_chr_access_dis,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            0, /* No more characteristics in this service */
        } },
    },

    {
        0, /* No more services */
    },
};

static int
gatt_svr_chr_access_dis(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *val;
    int rc;
    uint16_t uuid16;
    char tmp_buf[32 + 1]; ///hwid is only one that needs some tmp buffer

    char conf_name[11];

    if(ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }
    uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    assert(uuid16 != 0);

    switch (uuid16) {
        case BLE_SVC_DIS_CHR_SYS_ID_UUID16:
            strcpy(conf_name, "id/hwid");
            val = conf_get_value(conf_name, tmp_buf, sizeof(tmp_buf));
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_SVC_DIS_CHR_MODEL_NUM_UUID16:
            strcpy(conf_name, "id/app");
            val = conf_get_value(conf_name, NULL, 0);
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_SVC_DIS_CHR_SERIAL_NUM_UUID16:
            strcpy(conf_name, "id/serial");
            val = conf_get_value(conf_name, NULL, 0);
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_SVC_DIS_CHR_FW_REV_UUID16:
            strcpy(conf_name, "id/app");
            val = conf_get_value(conf_name, NULL, 0);
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_SVC_DIS_CHR_HW_REV_UUID16:
            strcpy(conf_name, "id/bsp");
            val = conf_get_value(conf_name, NULL, 0);
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_SVC_DIS_CHR_SW_REV_UUID16:
            strcpy(conf_name, "id/app");
            val = conf_get_value(conf_name, NULL, 0);
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_SVC_DIS_CHR_MFG_NAME_UUID16:
            strcpy(conf_name, "id/mfghash");
            val = conf_get_value(conf_name, NULL, 0);
            rc = os_mbuf_append(ctxt->om, val, strlen(val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}

/**
 * dis GATT server initialization
 *
 * @param eventq
 * @return 0 on success; non-zero on failure
 */
static int
dis_gatt_svr_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        goto err;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

err:
    return rc;
}

/**
 * dis console initialization
 *
 * @param Maximum input
 */
void
ble_svc_dis_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    /* Automatically register the service. */
    rc = dis_gatt_svr_init();
    SYSINIT_PANIC_ASSERT(rc == 0);
}
