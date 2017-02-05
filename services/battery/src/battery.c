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
#include "battery/battery.h"
#include "os/endian.h"

uint16_t ble_svc_battery_value;

/* battery attr read handle */
uint16_t g_battery_attr_read_handle;


static int
gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Battery */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_BATTERY_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_BATTERY_CHR_LEVEL_UUID16),
            .val_handle = &g_battery_attr_read_handle,
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        }, {
            0, /* No more characteristics in this service */
        } },
    },
    {
        0, /* No more services */
    },
};

static int
gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
            rc = os_mbuf_append(ctxt->om, &ble_svc_battery_value,
                                sizeof ble_svc_battery_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

/**
 * Battery GATT server initialization
 *
 * @param eventq
 * @return 0 on success; non-zero on failure
 */
int
battery_gatt_svr_init(void)
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
 * Update Battery level
 *
 * @param eventq
 * @return 0 on success; non-zero on failure
 */
void
battery_set_level(uint16_t battery_level)
{
    ble_svc_battery_value = battery_level;
    ble_gatts_chr_updated(g_battery_attr_read_handle);
}

/**
 * Battery service initialization
 *
 * @param Maximum input
 */
void
battery_init(void)
{
    ble_svc_battery_value = 0;

    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    /* Automatically register the service. */
    rc = battery_gatt_svr_init();
    SYSINIT_PANIC_ASSERT(rc == 0);
}
