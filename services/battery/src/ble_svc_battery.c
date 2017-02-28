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

#include "syscfg/syscfg.h"
#include "sysinit/sysinit.h"
#include "bsp/bsp.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "os/os_dev.h"
#include "battery/ble_svc_battery.h"
#include <adc/adc.h>

// #define ADC_NAME #MYNEWT_VAL(BATTERY_ADC_NAME)

/* ADC */
#include "adc/adc.h"

// /* ADC Task settings */
#define ADC_STACK_SIZE          (OS_STACK_ALIGN(32))
struct os_task ble_svc_battery_adc_task;
static bssnz_t os_stack_t ble_svc_battery_adc_stack[ADC_STACK_SIZE];

static struct adc_dev *ble_svc_battery_adc;

static uint16_t ble_svc_battery_value;

/* battery attr read handle */
static uint16_t battery_attr_read_handle;


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
            .val_handle = &battery_attr_read_handle,
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
static int
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

//stolen from nordic app_util.h
static __INLINE uint8_t battery_level_in_percent(const uint16_t mvolts)
{
    uint8_t battery_level;

    if (mvolts >= 3000)
    {
        battery_level = 100;
    }
    else if (mvolts > 2900)
    {
        battery_level = 100 - ((3000 - mvolts) * 58) / 100;
    }
    else if (mvolts > 2740)
    {
        battery_level = 42 - ((2900 - mvolts) * 24) / 160;
    }
    else if (mvolts > 2440)
    {
        battery_level = 18 - ((2740 - mvolts) * 12) / 300;
    }
    else if (mvolts > 2100)
    {
        battery_level = 6 - ((2440 - mvolts) * 6) / 340;
    }
    else
    {
        battery_level = 0;
    }

    return battery_level;
}

static int
ble_svc_battery_adc_read(void *buffer, int buffer_len)
{
    int i;
    int adc_result;
    int my_result_mv = 0;
    int rc;

    for (i = 0; i < MYNEWT_VAL(BATTERY_SAMPLES); i++) {
        rc = adc_buf_read(ble_svc_battery_adc, buffer, buffer_len, i, &adc_result);
        if (rc != 0) {
            goto err;
        }
        my_result_mv = adc_result_mv(ble_svc_battery_adc, 0, adc_result);
    }        
    adc_buf_release(ble_svc_battery_adc, buffer, buffer_len);
    return my_result_mv;
err:
    return (rc);
}

int
ble_svc_battery_adc_read_event(struct adc_dev *dev, void *arg, uint8_t etype,
        void *buffer, int buffer_len)
{
    int value = ble_svc_battery_adc_read(buffer, buffer_len);
    ble_svc_battery_value = battery_level_in_percent(value);
    return (0);
} 

static void
ble_svc_battery_adc_task_handler(void *unused)
{
    while (1) {
        adc_sample(ble_svc_battery_adc);
        /* Wait 30 min */
        os_time_delay(OS_TICKS_PER_SEC * MYNEWT_VAL(BATTERY_SAMPLE_DELAY));
    }
}

/**
 * Battery service initialization
 *
 * @param Maximum input
 */
void
ble_svc_battery_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    /* Automatically register the service. */
    rc = battery_gatt_svr_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

    /* ADC init */
    ble_svc_battery_adc = (struct adc_dev *)os_dev_lookup(MYNEWT_VAL(BATTERY_ADC_NAME));
    SYSINIT_PANIC_ASSERT(ble_svc_battery_adc != NULL);

    rc = adc_event_handler_set(ble_svc_battery_adc, ble_svc_battery_adc_read_event, (void *) NULL);
    SYSINIT_PANIC_ASSERT(rc == 0);

    // Kick off a sample
    rc = adc_sample(ble_svc_battery_adc);
    SYSINIT_PANIC_ASSERT(rc == 0);

    /* Create the battery_read reader task.
     * All sensor operations are performed in this task.
     */
    rc = os_task_init(&ble_svc_battery_adc_task, "battery", ble_svc_battery_adc_task_handler,
        NULL, MYNEWT_VAL(BATTERY_TASK_PRIO), OS_WAIT_FOREVER,
        ble_svc_battery_adc_stack, ADC_STACK_SIZE);
    SYSINIT_PANIC_ASSERT(rc == 0);
}
