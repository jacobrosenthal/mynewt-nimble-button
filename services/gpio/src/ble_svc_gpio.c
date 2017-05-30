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
#include "gpio/ble_svc_gpio.h"
#include "gpio/Arduino.h"
#include "gpio/Firmata.h"

/* Mynewt ADC driver */
#include <adc/adc.h>

/* Mynewt Nordic driver */
#include <adc_nrf51/adc_nrf51.h>

/* Nordic driver */
#include "nrf.h"
#include "app_util_platform.h"
#include "nrf_drv_adc.h"
#include "nrf_adc.h"

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

//everything is analog, if you beleive
#define IS_PIN_ANALOG(pin)  (true)

//everything is digital, if you beleive
#define IS_PIN_DIGITAL(pin)  (true)

//everything is pwm, if you beleive
#define IS_PIN_PWM(pin)  (true)

static const ble_uuid128_t gpio_uuid =
    BLE_UUID128_INIT(0x1b, 0xad, 0xa5, 0x55, 0xba, 0xda, 0x04, 0x20, 0x69, 0x69,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x01);


PinState_t states[TOTAL_PINS];

static uint8_t nimble_gpio_digital_value[2];
static uint16_t nimble_gpio_digital_value_len;

static uint8_t nimble_gpio_analog_value[3];
// static uint16_t nimble_gpio_analog_value_len;

static uint8_t nimble_gpio_config_value[3];
static uint16_t nimble_gpio_config_value_len;


/* Characteristic value handles */
static uint16_t ble_svc_gpio_digial_value_handle;
static uint16_t ble_svc_gpio_analog_value_handle;
static uint16_t ble_svc_gpio_config_value_handle;

// /* GPIO Task settings */
#define NIMBLE_GPIO_STACK_SIZE          (OS_STACK_ALIGN(96))
struct os_task gpio_task;
static bssnz_t os_stack_t gpio_stack[NIMBLE_GPIO_STACK_SIZE];

static struct adc_dev os_bsp_adc0;

static nrf_drv_adc_config_t os_bsp_adc0_config = {
    .interrupt_priority = MYNEWT_VAL(NIMBLE_GPIO_ADC_0_INTERRUPT_PRIORITY),
};

static nrf_drv_adc_channel_t cc = {{{
    .resolution           = MYNEWT_VAL(NIMBLE_GPIO_ADC_0_RESOLUTION),
    .input                = MYNEWT_VAL(NIMBLE_GPIO_ADC_0_SCALING),
    .reference            = MYNEWT_VAL(NIMBLE_GPIO_ADC_0_REFERENCE),
    .external_reference   = MYNEWT_VAL(NIMBLE_GPIO_ADC_0_EXTERNAL_REFERENCE),
}}, NULL};


uint16_t samplingInterval = 500;
// uint16_t samplingInterval = MYNEWT_VAL(NIMBLE_GPIO_SAMPLING_INTERVAL);

static void
gpio_task_handler(void *unused)
{
    struct adc_dev *adc_dev;
    int sample;
    int rc;
    int i;

    adc_dev = (struct adc_dev *) os_dev_open(MYNEWT_VAL(NIMBLE_GPIO_ADC_NAME), 0, NULL);
    assert(adc_dev != NULL);

    while (1) {
        for (i = 0; i < TOTAL_PINS; i++) {

            if (states[i].reportDigital) {
                int val = hal_gpio_read(i);

                if (states[i].value != val) {
                    nimble_gpio_digital_value[0] = i;
                    nimble_gpio_digital_value[1] = val;
                    ble_gatts_chr_updated(ble_svc_gpio_digial_value_handle);
                }
                states[i].value = val;
            }

            if (states[i].reportAnalog == 1) {

                // cc[0].ain = i; //TODO
                rc = adc_chan_config(adc_dev, 0, &cc);
                assert(rc == 0);

                adc_chan_read(adc_dev, 0, &sample);
                assert(rc == 0);

                nimble_gpio_analog_value[0] = i;
                nimble_gpio_analog_value[1] = lowByte(sample);
                nimble_gpio_analog_value[2] = highByte(sample);
                ble_gatts_chr_updated(ble_svc_gpio_analog_value_handle);
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
        .uuid = &gpio_uuid.u,
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
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_WRITE_NO_RSP,
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

static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
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
    uint16_t newInterval;

    uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    assert(uuid16 != 0);

    switch (uuid16) {

    case BLE_SVC_GPIO_CHR_UUID16_DIGITAL_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &nimble_gpio_digital_value,
                                sizeof nimble_gpio_digital_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {

            rc = gatt_svr_chr_write(ctxt->om, 0,
                                    sizeof nimble_gpio_digital_value,
                                    nimble_gpio_digital_value,
                                    &nimble_gpio_digital_value_len);

            uint8_t p = nimble_gpio_digital_value[0];
            uint8_t v = nimble_gpio_digital_value[1];

            if (IS_PIN_DIGITAL(p)) {
                if (v > 0) {
                    hal_gpio_write(p, 1);
                } else {
                    hal_gpio_write(p, 0);
                }
            }
            return rc;

        } else {
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    case BLE_SVC_GPIO_CHR_UUID16_ANALOG_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &nimble_gpio_analog_value,
                                sizeof nimble_gpio_analog_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else {
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }

    case BLE_SVC_GPIO_CHR_UUID16_CONFIG_STAT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, &nimble_gpio_config_value,
                                sizeof nimble_gpio_config_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {


            rc = gatt_svr_chr_write(ctxt->om, 0,
                                    sizeof nimble_gpio_config_value,
                                    nimble_gpio_config_value,
                                    &nimble_gpio_config_value_len);

            uint8_t cmd = nimble_gpio_config_value[0];
            uint8_t p = nimble_gpio_config_value[1];
            uint8_t val = nimble_gpio_config_value[2];

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
                newInterval = (uint16_t) p;
                if (nimble_gpio_config_value_len > 2) {
                    newInterval += val << 8;
                }
                samplingInterval = newInterval;
                break;

            default:
                assert(0);
                return BLE_ATT_ERR_UNLIKELY;
            }
            return rc;
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
    int i;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = ble_gatts_count_cfg(ble_svc_gpio_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_gatts_add_svcs(ble_svc_gpio_defs);
    SYSINIT_PANIC_ASSERT(rc == 0);

    //this would probably be in the bsp except the licensing issues?
    rc = os_dev_create((struct os_dev *) &os_bsp_adc0, MYNEWT_VAL(NIMBLE_GPIO_ADC_NAME),
            OS_DEV_INIT_KERNEL, OS_DEV_INIT_PRIO_DEFAULT,
            nrf51_adc_dev_init, &os_bsp_adc0_config);
    SYSINIT_PANIC_ASSERT(rc == 0);

    for (i = 0; i < TOTAL_PINS; i++) {
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
                      NULL, MYNEWT_VAL(NIMBLE_GPIO_TASK_PRIO), OS_WAIT_FOREVER,
                      gpio_stack, NIMBLE_GPIO_STACK_SIZE);
    SYSINIT_PANIC_ASSERT(rc == 0);
}
