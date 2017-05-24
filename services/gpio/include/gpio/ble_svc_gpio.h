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

#ifndef H_BLE_SVC_GPIO_
#define H_BLE_SVC_GPIO_

#ifdef __cplusplus
extern "C" {
#endif

struct PinState {
  uint8_t mode;
  uint16_t value;
  bool reportDigital;
  bool reportAnalog;
  bool pwm;
};

/* 16 Bit Alert Notification Servivce Characteristic UUIDs */
#define BLE_SVC_GPIO_CHR_UUID16_DIGITAL_STAT                	0x2a56
#define BLE_SVC_GPIO_CHR_UUID16_ANALOG_STAT                  	0x2a58
#define BLE_SVC_GPIO_CHR_UUID16_CONFIG_STAT                  	0x2a59

void ble_svc_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_SVC_GPIO_ */
