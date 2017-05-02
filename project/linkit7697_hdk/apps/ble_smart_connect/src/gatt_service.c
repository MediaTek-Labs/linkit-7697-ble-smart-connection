/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "bt_uuid.h"
#include "bt_system.h"
#include "bt_gattc.h"
#include "bt_gatt.h"
#include "bt_gatts.h"
#include <string.h>
#include "project_config.h"

//Declare every record here
//service collects all bt_gatts_service_rec_t
//IMPORTAMT: handle:0x0000 is reserved, please start your handle from 0x0001

//GATT 0x0014 - 0x0017
/*---------------------------------------------*/
const bt_uuid_t APP_BLE_SMTCN_CHAR_UUID128 = BT_UUID_INIT_WITH_UUID16(APP_BLE_SMTCN_CHAR_UUID);

extern uint32_t app_ble_smtcn_charc_value_callback (const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
extern uint32_t app_ble_smtcn_client_config_callback (const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

BT_GATTS_NEW_PRIMARY_SERVICE_16(_bt_if_dtp_primary_service, APP_BLE_SMTCN_SERVICE_UUID);

BT_GATTS_NEW_CHARC_16(_bt_if_dtp_char,
                      BT_GATT_CHARC_PROP_WRITE | BT_GATT_CHARC_PROP_INDICATE, APP_BLE_SMTCN_CHAR_VALUE_HANDLE, APP_BLE_SMTCN_CHAR_UUID);

BT_GATTS_NEW_CHARC_VALUE_CALLBACK(_bt_if_dtp_char_value, APP_BLE_SMTCN_CHAR_UUID128,
                                  BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE, app_ble_smtcn_charc_value_callback);

BT_GATTS_NEW_CLIENT_CHARC_CONFIG(_bt_if_dtp_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE,
                                 app_ble_smtcn_client_config_callback);

static const bt_gatts_service_rec_t *_bt_if_ble_smtcn_service_rec[] = {
    (const bt_gatts_service_rec_t *) &_bt_if_dtp_primary_service,
    (const bt_gatts_service_rec_t *) &_bt_if_dtp_char,
    (const bt_gatts_service_rec_t *) &_bt_if_dtp_char_value,
    (const bt_gatts_service_rec_t *) &_bt_if_dtp_client_config
};

static const bt_gatts_service_t _bt_if_ble_smtcn_service = {
    .starting_handle = 0x0014,
    .ending_handle = 0x0017,
    .required_encryption_key_size = 0,
    .records = _bt_if_ble_smtcn_service_rec
};

//server collects all service
static const bt_gatts_service_t * _bt_if_gatt_server[] = {
    //&_bt_if_gap_service,//0x0001
    //&_bt_if_gatt_service_ro,//0x0011
    &_bt_if_ble_smtcn_service, //0x0014-0x0017
    NULL
    };




//When GATTS get req from remote client, GATTS will call bt_get_gatt_server() to get application's gatt service DB.
//You have to return the DB(bt_gatts_service_t pointer) to gatts stack.
const bt_gatts_service_t** bt_get_gatt_server()
{
    return _bt_if_gatt_server;
}

