/**
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
#include <string.h>
#include <math.h>
#include "sysinit/sysinit.h"
#include "syscfg/syscfg.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "button/ble_svc_button.h"


/* Characteristic values */
static uint32_t ble_svc_button_led_value;
static uint16_t ble_svc_button_led_value_handle;

/* Charachteristic value handles */
static uint32_t ble_svc_button_button_value;
static uint16_t ble_svc_button_button_value_handle;

button_svc_written_ptr ble_svc_button_written_cb;

/* Access function */
static int
ble_svc_button_access(uint16_t conn_handle, uint16_t attr_handle,
                   struct ble_gatt_access_ctxt *ctxt, void *arg);


/* Save written value to local characteristic value */
static int
ble_svc_button_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, 
                      void *dst, uint16_t *len);

static const struct ble_gatt_svc_def ble_svc_button_defs[] = {
    {
        /*** Alert Notification Service. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_BUTTON_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_BUTTON_CHR_UUID16_BUTTON_STAT),
            .access_cb = ble_svc_button_access,
            .val_handle = &ble_svc_button_button_value_handle,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_SVC_BUTTON_CHR_UUID16_LED_STAT),
            .access_cb = ble_svc_button_access,
            .val_handle = &ble_svc_button_led_value_handle,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_WRITE_NO_RSP,
        }, {
            0, /* No more characteristics in this service. */
        } },
    },

    {
        0, /* No more services. */
    },
};

/**
 * ANS access function
 */
static int
ble_svc_button_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt,
                          void *arg)
{
    uint16_t uuid16;
    int rc;
    
    /* ANS Control point command and catagory variables */

    uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    assert(uuid16 != 0);

    switch (uuid16) {

    case BLE_SVC_BUTTON_CHR_UUID16_BUTTON_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &ble_svc_button_button_value,
                                sizeof ble_svc_button_button_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }else{
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    case BLE_SVC_BUTTON_CHR_UUID16_LED_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &ble_svc_button_led_value,
                                sizeof ble_svc_button_led_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // if (OS_MBUF_PKTLEN(ctxt->om) != sizeof ble_svc_button_led_value) {
            //     return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            // }

            rc = ble_svc_button_chr_write(ctxt->om, 0,
                                       sizeof ble_svc_button_led_value,
                                       &ble_svc_button_led_value,
                                       NULL);

            if(ble_svc_button_written_cb){
                ble_svc_button_written_cb(ctxt->chr->uuid, ble_svc_button_led_value);
            }

            return rc;
        }

    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
  return 0;
}

/**
 * Writes the received value from a characteristic write to 
 * the given destination.
 */
static int
ble_svc_button_chr_write(struct os_mbuf *om, uint16_t min_len,
                      uint16_t max_len, void *dst,
                      uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}


void
ble_svc_button_set_led(uint32_t level)
{
    ble_svc_button_led_value = level;
    ble_gatts_chr_updated(ble_svc_button_led_value_handle);
}

void
ble_svc_button_set_button(uint32_t level)
{
    ble_svc_button_button_value = level;
    ble_gatts_chr_updated(ble_svc_button_button_value_handle);
}

void
ble_svc_button_set_written_callback(button_svc_written_ptr cb)
{
    ble_svc_button_written_cb = cb;
}

/**
 * Initialize the ANS with initial values for enabled categories
 * for new and unread alert characteristics. Bitwise or the 
 * catagory bitmasks to enable multiple catagories.
 * 
 * XXX: We should technically be able to change the new alert and
 *      unread alert catagories when we have no active connections.
 * 
 * @return 0 on success, non-zero error code otherwise.
 */
void
ble_svc_button_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = ble_gatts_count_cfg(ble_svc_button_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_gatts_add_svcs(ble_svc_button_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);
}
