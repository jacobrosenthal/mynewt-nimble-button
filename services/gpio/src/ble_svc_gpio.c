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
#include "bsp/bsp.h"
#include "os/os.h"
#include "hal/hal_gpio.h"
#include "host/ble_hs.h"
#include "button/ble_svc_gpio.h"
#include "Arduino.h"
#include "Firmata.h"

#include "adc_nrf51_driver/adc_nrf51_driver.h"

/* Mynewt ADC driver */
#include <adc/adc.h>

/* Mynewt Nordic driver */
#include <adc_nrf51/adc_nrf51.h>

/* Nordic driver */
#include "nrf.h"
#include "app_util_platform.h"
#include "nrf_drv_adc.h"
#include "nrf_adc.h"

//everything is analog, if you beleive
#define IS_PIN_ANALOG(pin)  (true)

//everything is pwm, if you beleive
#define IS_PIN_PWM(pin)  (true)

static const ble_uuid128_t gpio_uuid =
    BLE_UUID128_INIT(0x1b, 0xad, 0xa5, 0x55, 0xba, 0xda, 0x04, 0x20, 0x69, 0x69,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x01);

PinState states[TOTAL_PINS];

static struct os_event advertise_handle_event;

/* Characteristic value handles */
static uint16_t ble_svc_gpio_digial_value_handle;
static uint16_t ble_svc_gpio_analog_value_handle;
static uint16_t ble_svc_gpio_config_value_handle;

// /* GPIO Task settings */
#define GPIO_STACK_SIZE          (OS_STACK_ALIGN(48))
struct os_task gpio_task;
static bssnz_t os_stack_t gpio_stack[GPIO_STACK_SIZE];

static nrf_drv_adc_config_t os_bsp_adc0_config = {
    .interrupt_priority = MYNEWT_VAL(ADC_0_INTERRUPT_PRIORITY),
};

static nrf_drv_adc_channel_t cc = {{{
            .resolution           = MYNEWT_VAL(ADC_0_RESOLUTION),
            .input                = MYNEWT_VAL(ADC_0_SCALING),
            .reference            = MYNEWT_VAL(ADC_0_REFERENCE),
            .external_reference   = MYNEWT_VAL(ADC_0_EXTERNAL_REFERENCE),
        }
    }, NULL
};

uint16_t samplingInterval = 500;
// uint16_t samplingInterval = MYNEWT_VAL(NIMBLE_GPIO_SAMPLING_INTERVAL);

static void
gpio_task_handler(void *unused)
{
    struct adc_dev *adc;
    int8_t sample;
    int rc;

    //this would probably be in the bsp except the licensing issues?
    rc = os_dev_create((struct os_dev *) &os_bsp_adc0, "adc0",
                       OS_DEV_INIT_KERNEL, OS_DEV_INIT_PRIO_DEFAULT,
                       nrf51_adc_dev_init, &os_bsp_adc0_config);
    assert(rc == 0);

    adc_dev = (struct adc_dev *) os_dev_open("adc0", 0, &os_bsp_adc0_config);
    assert(adc_dev != NULL);

    while (1) {
        for (int i = 0; i < TOTAL_PINS; i++) {

            if (states[i].reportDigital) {
                int val = hal_gpio_read(i);

                if (states[i].value != val) {
                    unsigned char report[2] = {(unsigned char)i, (unsigned char)val};
                    // digitalChar.setValue(report, 2);
                }
                states[i].value = val;
            }

            if (notify == 1 && states[i].reportAnalog == 1) {

                cc.ain = i;
                rc = adc_chan_config(adc_dev, 0, &cc);
                assert(rc == 0);

                adc_chan_read(adc, 0, sample);
                assert(rc == 0);

                unsigned char report[3] = {(unsigned char)i, lowByte(sample), highByte(sample)};
                // analogChar.setValue(report, 3);

            }

        }
        os_time_delay((samplingInterval * OS_TICKS_PER_SEC) / 1000);
    }
}

/* Access function */
static int
ble_svc_gpio_access(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def ble_svc_gpio_defs[] = {
    {
        /*** GPIO Service. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = gpio_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_GPIO_CHR_UUID16_DIGITAL_STAT),
                .access_cb = ble_svc_gpio_access,
                .val_handle = &ble_svc_gpio_digial_value_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_WRITE_NO_RSP,
            }, {
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_GPIO_CHR_UUID16_ANALOG_STAT),
                .access_cb = ble_svc_gpio_access,
                .val_handle = &ble_svc_gpio_analog_value_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, | BLE_GATT_CHR_F_WRITE_NO_RSP,
            }, {
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_GPIO_CHR_UUID16_CONFIG_STAT),
                .access_cb = ble_svc_gpio_access,
                .val_handle = &ble_svc_gpio_config_value_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP,
            }, {
                0, /* No more characteristics in this service. */
            }
        },
    },
    {
        0, /* No more services. */
    },
};

/**
 * GPIO access function
 */
static int
ble_svc_gpio_access(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg)
{
    uint16_t uuid16;
    int rc;

    uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    assert(uuid16 != 0);

    switch (uuid16) {

    case BLE_SVC_GPIO_CHR_UUID16_DIGITAL_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &g_stats_gpio_toggle.stoggles,
                                sizeof g_stats_gpio_toggle.stoggles);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {

            uint8_t p = digitalChar.value()[0];
            uint8_t v = digitalChar.value()[1];
            if (IS_PIN_DIGITAL(p)) {
                if (v > 0) {
                    hal_gpio_write(p, 1)
                } else {
                    hal_gpio_write(p, 0);
                }
            }

        } else {
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    case BLE_SVC_GPIO_CHR_UUID16_ANALOG_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &g_stats_gpio_toggle.stoggles,
                                sizeof g_stats_gpio_toggle.stoggles);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else {
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    case BLE_SVC_GPIO_CHR_UUID16_CONFIG_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &g_stats_gpio_toggle.stoggles,
                                sizeof g_stats_gpio_toggle.stoggles);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {


            uint8_t cmd = configChar.value()[0];
            uint8_t p = configChar.value()[1];
            uint8_t val = configChar.value()[2];

            switch (cmd) {
            case SET_PIN_MODE:

                if (val == INPUT) {
                    hal_gpio_init_in(p, HAL_GPIO_PULL_NONE);
                } else if (val == INPUT_PULLUP) {
                    hal_gpio_init_in(p, HAL_GPIO_PULL_UP);
                } else if (val == OUTPUT) {
                    hal_gpio_init_out(p, 0);
                } else {
                    return BLE_ATT_ERR_UNLIKELY;
                }
                break;

            case REPORT_ANALOG:
                if (IS_PIN_ANALOG(p)) {
                    states[p].reportAnalog = (val == 0) ? 0 : 1;
                    states[p].reportDigital = !states[p].reportAnalog;
                } else {
                    return BLE_ATT_ERR_UNLIKELY;
                }
                break;

            case REPORT_DIGITAL:
                if (IS_PIN_DIGITAL(p)) {
                    states[p].reportDigital = (val == 0) ? 0 : 1;
                    states[p].reportAnalog = !states[p].reportDigital;
                } else {
                    return BLE_ATT_ERR_UNLIKELY;
                }
                break;

            case SAMPLING_INTERVAL:
                uint16_t newInterval = (uint16_t) p;
                if (len > 2) {
                    newInterval += val << 8;
                }
                samplingInterval = newInterval;
                break;

            default:
                assert(0);
                return BLE_ATT_ERR_UNLIKELY;
            }
        } else {
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}

void
ble_svc_gpio_init(void)
{
    int rc;

    ble_gatts_chr_updated(ble_svc_gpio_analog_value_handle);
    ble_gatts_chr_updated(ble_svc_gpio_digial_value_handle);

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = ble_gatts_count_cfg(ble_svc_gpio_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_gatts_add_svcs(ble_svc_gpio_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    for (int i = 0; i < TOTAL_PINS; i++) {
        states[i].pwm = true;
        states[i].value = 0;
        states[i].reportAnalog = 0;
        states[i].reportDigital = 0;
        states[i].mode = OUTPUT;
    }

    /* Create the button reader task.
     * All sensor operations are performed in this task.
     */
    rc = os_task_init(&gpio_task, "gpio", gpio_task_handler,
                      NULL, MYNEWT_VAL(GPIO_TASK_PRIO), OS_WAIT_FOREVER,
                      gpio_stack, GPIO_STACK_SIZE);
    SYSINIT_PANIC_ASSERT(rc == 0);
}
