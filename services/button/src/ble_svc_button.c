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
#include "sysinit/sysinit.h"
#include "syscfg/syscfg.h"
#include "stats/stats.h"
#include "bsp/bsp.h"
#include "os/os.h"
#include "hal/hal_gpio.h"
#include "host/ble_hs.h"
#include "button/ble_svc_button.h"

static struct os_event advertise_handle_event;

/* Characteristic value handles */
static uint16_t ble_svc_button_button_value_handle;

//lets store our button as a stat so we can access it that way too
STATS_SECT_START(gpio_stats)
STATS_SECT_ENTRY(toggles)
STATS_SECT_END

static STATS_SECT_DECL(gpio_stats) g_stats_gpio_toggle;

static STATS_NAME_START(gpio_stats)
STATS_NAME(gpio_stats, toggles)
STATS_NAME_END(gpio_stats)

// /* Button Task settings */
#define BUTTON_STACK_SIZE          (OS_STACK_ALIGN(48))
struct os_task button_task;
static bssnz_t os_stack_t button_stack[BUTTON_STACK_SIZE];

static int last;
static bool pressed;

//implementing button on 2 consecutive low reads
static void
button_task_handler(void *unused)
{
    while (1) {
        int current = hal_gpio_read(MYNEWT_VAL(BUTTON_PIN));

#if MYNEWT_VAL(BUTTON_INVERTED)
    {
        current=!current;
    }
#endif

        if( !pressed && current && last )
        {
            pressed = true;
            STATS_INC(g_stats_gpio_toggle, toggles);
            ble_gatts_chr_updated(ble_svc_button_button_value_handle);

            //keep stack small, trigger callback on the default queue
            if (advertise_handle_event.ev_cb)
            {
                advertise_handle_event.ev_arg = &pressed;
                os_eventq_put(os_eventq_dflt_get(), &advertise_handle_event);
            }

        }else if(pressed && last != current)
        {
            pressed = false;
        }
        last = current;

        /* Wait 50 ms */
        os_time_delay((50 * OS_TICKS_PER_SEC) / 1000);
    }
}

/* Access function */
static int
ble_svc_button_access(uint16_t conn_handle, uint16_t attr_handle,
                   struct ble_gatt_access_ctxt *ctxt, void *arg);

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
            0, /* No more characteristics in this service. */
        } },
    },

    {
        0, /* No more services. */
    },
};

/**
 * Button access function
 */
static int
ble_svc_button_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt,
                          void *arg)
{
    uint16_t uuid16;
    int rc;
    
    /* Button Control point command and catagory variables */

    uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    assert(uuid16 != 0);

    switch (uuid16) {

    case BLE_SVC_BUTTON_CHR_UUID16_BUTTON_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &g_stats_gpio_toggle.stoggles,
                                sizeof g_stats_gpio_toggle.stoggles);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }else{
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
  return 0;
}


void ble_svc_button_register_handler(os_event_fn *cb)
{
    advertise_handle_event.ev_cb = cb;
}

uint32_t
ble_svc_button_count(void)
{
    return g_stats_gpio_toggle.stoggles;
}

void
ble_svc_button_init(void)
{
    int rc;

    hal_gpio_init_in(MYNEWT_VAL(BUTTON_PIN), MYNEWT_VAL(BUTTON_PULLUP));

    stats_init(STATS_HDR(g_stats_gpio_toggle),
               STATS_SIZE_INIT_PARMS(g_stats_gpio_toggle, STATS_SIZE_32),
               STATS_NAME_INIT_PARMS(gpio_stats));

    stats_register("gpio_toggle", STATS_HDR(g_stats_gpio_toggle));

    ble_gatts_chr_updated(ble_svc_button_button_value_handle);

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = ble_gatts_count_cfg(ble_svc_button_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_gatts_add_svcs(ble_svc_button_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    /* Create the button reader task.
     * All sensor operations are performed in this task.
     */
    rc = os_task_init(&button_task, "button", button_task_handler,
            NULL, MYNEWT_VAL(BUTTON_TASK_PRIO), OS_WAIT_FOREVER,
            button_stack, BUTTON_STACK_SIZE);
    SYSINIT_PANIC_ASSERT(rc == 0);
}
