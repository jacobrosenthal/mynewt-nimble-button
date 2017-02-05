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

#ifndef H_BLE_SVC_BUTTON_
#define H_BLE_SVC_BUTTON_

/* 16 Bit Alert Notification Service UUID */
#define BLE_SVC_BUTTON_UUID16                                  0xA000

/* 16 Bit Alert Notification Servivce Characteristic UUIDs */
#define BLE_SVC_BUTTON_CHR_UUID16_LED_STAT			           0xA002
#define BLE_SVC_BUTTON_CHR_UUID16_BUTTON_STAT                  0xA001

void ble_svc_button_on_gap_connect(uint16_t conn_handle);

int ble_svc_button_set_button(uint16_t level);
int ble_svc_button_set_led(uint16_t level);

void ble_svc_button_init(void);

#endif

