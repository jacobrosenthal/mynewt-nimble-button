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

#ifndef _BLEDIS_H_
#define _BLEDIS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_SVC_DIS_UUID16                                  0x180A
#define BLE_SVC_DIS_CHR_SYS_ID_UUID16  		                0x2A23
#define BLE_SVC_DIS_CHR_MODEL_NUM_UUID16                    0x2A24
#define BLE_SVC_DIS_CHR_SERIAL_NUM_UUID16                   0x2A25
#define BLE_SVC_DIS_CHR_FW_REV_UUID16                       0x2A26
#define BLE_SVC_DIS_CHR_HW_REV_UUID16                     	0x2A27
#define BLE_SVC_DIS_CHR_SW_REV_UUID16                       0x2A28
#define BLE_SVC_DIS_CHR_MFG_NAME_UUID16                     0x2A29

void
dis_init(void);
int
dis_gatt_svr_init(void);


#ifdef __cplusplus
}
#endif

#endif /* _BLEDIS_H */
