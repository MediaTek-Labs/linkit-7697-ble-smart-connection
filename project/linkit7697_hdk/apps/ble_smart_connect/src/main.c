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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* device.h includes */
#include "mt7687.h"
#include "system_mt7687.h"

/* wifi related header */
#include "wifi_api.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"

/* ble related header */
#include "bt_init.h"
#include "bt_gatts.h"
#include "bt_gap_le.h"
#include "bt_uuid.h"
#include "bt_debug.h"

/* app-wise config */
#include "project_config.h"
#include "sys_init.h"
#include "task_def.h"

/* ---------------------------------------------------------------------------- */

/* Create the log control block as user wishes. Here we use 'app' as module name.
 * User needs to define their own log control blocks as project needs.
 * Please refer to the log dev guide under /doc folder for more details.
 */
log_create_module(app, PRINT_LEVEL_INFO);

/* ---------------------------------------------------------------------------- */

#define APP_BLE_SMTCN_MAX_INTERVAL          0x00C0    /*The range is from 0x0020 to 0x4000.*/
#define APP_BLE_SMTCN_MIN_INTERVAL          0x00C0    /*The range is from 0x0020 to 0x4000.*/
#define APP_BLE_SMTCN_CHANNEL_NUM           7
#define APP_BLE_SMTCN_FILTER_POLICY         0
#define APP_BLE_SMTCN_AD_FLAG_LEN           2
#define APP_BLE_SMTCN_AD_UUID_LEN           3


static void app_start_advertising(void)
{
    // start advertising
    bt_hci_cmd_le_set_advertising_enable_t enable;
    bt_hci_cmd_le_set_advertising_parameters_t adv_param = {
            .advertising_interval_min = APP_BLE_SMTCN_MIN_INTERVAL,
            .advertising_interval_max = APP_BLE_SMTCN_MAX_INTERVAL,
            .advertising_type = BT_HCI_ADV_TYPE_CONNECTABLE_UNDIRECTED,
            .own_address_type = BT_ADDR_RANDOM,
            .advertising_channel_map = APP_BLE_SMTCN_CHANNEL_NUM,
            .advertising_filter_policy = APP_BLE_SMTCN_FILTER_POLICY
        };
    bt_hci_cmd_le_set_advertising_data_t adv_data;

    adv_data.advertising_data[0] = APP_BLE_SMTCN_AD_FLAG_LEN;   // 2
    adv_data.advertising_data[1] = BT_GAP_LE_AD_TYPE_FLAG;      // 0x01
    adv_data.advertising_data[2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED | BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;

    adv_data.advertising_data[3] = APP_BLE_SMTCN_AD_UUID_LEN;
    adv_data.advertising_data[4] = BT_GAP_LE_AD_TYPE_16_BIT_UUID_COMPLETE;
    adv_data.advertising_data[5] = APP_BLE_SMTCN_SERVICE_UUID & 0x00FF;
    adv_data.advertising_data[6] = (APP_BLE_SMTCN_SERVICE_UUID & 0xFF00)>>8;

    adv_data.advertising_data[7] = 1+strlen(APP_BLE_SMTCN_DEVICE_NAME);
    adv_data.advertising_data[8] = BT_GAP_LE_AD_TYPE_NAME_COMPLETE;
    memcpy(adv_data.advertising_data+9, APP_BLE_SMTCN_DEVICE_NAME, strlen(APP_BLE_SMTCN_DEVICE_NAME));

    adv_data.advertising_data_length = 9 + strlen(APP_BLE_SMTCN_DEVICE_NAME);

    enable.advertising_enable = BT_HCI_ENABLE;
    bt_gap_le_set_advertising(&enable, &adv_param, &adv_data, NULL);
}

extern bt_bd_addr_t local_public_addr;

// called by bt_app_event_callback@bt_common.c
bt_status_t app_bt_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    LOG_I(app, "---> bt_event_callback(0x%08X,%d)", msg, status);

    switch(msg)
    {
    case BT_POWER_ON_CNF:
        LOG_I(app, "[BT_POWER_ON_CNF](%d)", status);

        // set random address before advertising
        LOG_I(app, "bt_gap_le_set_random_address()");    
        bt_gap_le_set_random_address((bt_bd_addr_ptr_t)local_public_addr);
        break;

    case BT_GAP_LE_SET_RANDOM_ADDRESS_CNF: 
        LOG_I(app, "[BT_GAP_LE_SET_RANDOM_ADDRESS_CNF](%d)", status);

        // start advertising
        app_start_advertising();
        break;

    case BT_GAP_LE_SET_ADVERTISING_CNF:
        LOG_I(app, "[BT_GAP_LE_SET_ADVERTISING_CNF](%d)", status);
        break;

    case BT_GAP_LE_DISCONNECT_IND:
        LOG_I(app, "[BT_GAP_LE_DISCONNECT_IND](%d)", status);

        // start advertising
        app_start_advertising();
        break;

    case BT_GAP_LE_CONNECT_IND:
        LOG_I(app, "[BT_GAP_LE_CONNECT_IND](%d)", status);

        bt_gap_le_connection_ind_t *connection_ind = (bt_gap_le_connection_ind_t *)buff;
        LOG_I(app, "-> connection handle = 0x%04x, role = %s", connection_ind->connection_handle, (connection_ind->role == BT_ROLE_MASTER)? "master" : "slave");

        LOG_I(app, "************************");
        LOG_I(app, "BLE connected!!");
        LOG_I(app, "************************");
        break;
    }

    LOG_I(app, "<--- bt_event_callback(0x%08X,%d)", msg, status);
    return BT_STATUS_SUCCESS;
}

/* ---------------------------------------------------------------------------- */

typedef enum {
    _WIFI_INFO_SSID = 0x01,
    _WIFI_INFO_PW,
    _WIFI_INFO_SEC_MODE,
    _WIFI_INFO_IP,

    _WIFI_INFO_DISCONNECTED = 0x07,
    _WIFI_INFO_CONNECTED = 0x08
} _wifi_info_id_t;

static uint16_t g_client_conn_handle = 0;
static uint16_t g_client_config_value = 0;

/* receivied wifi infomation */
static uint8_t g_rx_flag = 0;
static uint8_t g_ssid[21];
static uint8_t g_ssid_len;
static uint8_t g_pwd[21];
static uint8_t g_pwd_len;
static wifi_auth_mode_t g_auth;
static wifi_encrypt_type_t g_encrypt;

uint32_t app_ble_smtcn_client_config_callback (const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    LOG_I(app, "app_ble_smtcn_client_config_callback(rw=%d, handle=%d, data=%x, size=%d, offset=%d)", rw, handle, data, size, offset);

    g_client_conn_handle = handle;

    if (size != sizeof(g_client_config_value)){ //Size check
        return 0;
    }

    if (rw == BT_GATTS_CALLBACK_WRITE) 
    {
        g_client_config_value = *(uint16_t*)data;
        LOG_I(app, "g_client_config_value = %d now", g_client_config_value);
    }
    else 
    {
        memcpy(data, &g_client_config_value, sizeof(g_client_config_value));
    }

    return sizeof(g_client_config_value);
}

uint32_t app_ble_smtcn_charc_value_callback (const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    LOG_I(app, "app_ble_smtcn_charc_value_callback(rw=%d, handle=%d, data=%x, size=%d, offset=%d)", rw, handle, data, size, offset);

    uint8_t* _data = (uint8_t*)data;

    if (rw == BT_GATTS_CALLBACK_WRITE) 
    {
        _wifi_info_id_t     id  = _data[0];
        uint8_t             len = _data[1];

        switch(id)
        {
        case _WIFI_INFO_SSID:
            if(len > 20)
            {
                LOG_E(app, "error!!");
                return 0;
            }
            g_rx_flag |= (1<<id);
            memcpy(g_ssid, _data+2, len);
            g_ssid[len] = 0;
            g_ssid_len = len;
            LOG_I(app, "receive SSID=%s, flag=%X", g_ssid, g_rx_flag);
            break;
        case _WIFI_INFO_PW:
            if(len > 20)
            {
                LOG_E(app, "error!!");
                return 0;
            }
            g_rx_flag |= (1<<id);
            memcpy(g_pwd, _data+2, len);
            g_pwd[len] = 0;
            g_pwd_len = len;
            LOG_I(app, "receive PWD=%s, flag=%X", g_pwd, g_rx_flag);
            break;
        case _WIFI_INFO_SEC_MODE:
            if(len != 2)
            {
                LOG_E(app, "error!!");
                return 0;
            }
            g_rx_flag |= (1<<id);
            g_auth = (wifi_auth_mode_t)_data[2];  
            g_encrypt = (wifi_encrypt_type_t)_data[3];
            LOG_I(app, "receive auth=%d, enc=%d, flag=%X", g_auth, g_encrypt, g_rx_flag);
            break;
        }

        // all infomation ready, reload wifi settings
        if(g_rx_flag == ((1<<_WIFI_INFO_SSID)|(1<<_WIFI_INFO_PW)|(1<<_WIFI_INFO_SEC_MODE)))
        {
            int32_t result;
            uint8_t port = WIFI_PORT_STA;

            LOG_I(app, "************************");
            LOG_I(app, "Receive all wifi settings! [%s][%s][%d][%d]", g_ssid, g_pwd, g_auth, g_encrypt);
            LOG_I(app, "************************");
            result = wifi_config_set_ssid(port, g_ssid, g_ssid_len);
            if (result < 0) {
                LOG_E(app, "Error:wifi_config_set_ssid()=%d", result);
                return 0;
            }
            
            result = wifi_config_set_security_mode(port, g_auth, g_encrypt);
            if (result < 0) {
                LOG_E(app, "Error:wifi_config_set_security_mode()=%d", result);
                return 0;
            }
            
            if (g_encrypt == 0) 
            {
                // WEP not handled
                LOG_E(app, "WEP not handled!!");
            }
            else 
            {
                result = wifi_config_set_wpa_psk_key(port, g_pwd, g_pwd_len);
                if (result < 0) {
                    LOG_E(app, "Error:wifi_config_set_wpa_psk_key()=%d", result);
                    return 0;
                }
            }
           
            result = wifi_config_reload_setting();
            if (result < 0) {
                LOG_E(app, "Error:wifi_config_reload_setting()=%d", result);
                return 0;
            }
        }
    }
    else
    {
        // not support now
        return 0;
    }

    return size;
}

static void _send_ble_indication(_wifi_info_id_t id, uint8_t *data, uint8_t data_len)
{
    uint8_t buf[64] = {0};
    uint8_t pak[20] = {0}; 
    uint8_t pak_len;
    bt_gattc_charc_value_notification_indication_t *req;
    bt_status_t result;

    LOG_I(app, "_send_ble_indication, id=%d, data_len=%d", id, data_len);

    if(!g_client_conn_handle)
    {
        LOG_I(app, "Skipped _send_ble_indication() due to no client connection");
        return;
    }

    pak[0] = id;
    pak_len = 1;

    if (data_len > 18) {
        LOG_E(app, "data length too long !!");
        return;
    }

    if(data_len > 0)
    {
        pak_len = 2+data_len;
        pak[1] = data_len;
        memcpy(pak+2, data, data_len);
    }

    req = (bt_gattc_charc_value_notification_indication_t*)buf;
    req->attribute_value_length = 3 + pak_len;
    req->att_req.opcode = BT_ATT_OPCODE_HANDLE_VALUE_INDICATION;
    req->att_req.handle = APP_BLE_SMTCN_CHAR_VALUE_HANDLE;
    memcpy(req->att_req.attribute_value, pak, pak_len);
    result = bt_gatts_send_charc_value_notification_indication(g_client_conn_handle, req);

    LOG_I(app, "_send_ble_indication result = %d", result);
}

static void _ip_ready_callback(struct netif *netif)
{
    if (!ip4_addr_isany_val(netif->ip_addr)) 
    {
        if (NULL != inet_ntoa(netif->ip_addr)) 
        {
            char ip_addr[17] = {0};
            strcpy(ip_addr, inet_ntoa(netif->ip_addr));
            LOG_I(app, "************************");
            LOG_I(app, "DHCP got IP: %s", ip_addr);
            LOG_I(app, "************************");

            _send_ble_indication(_WIFI_INFO_IP, ip_addr, strlen(ip_addr));
        } else {
            LOG_E(app, "DHCP got IP failed");
        }
    }
    LOG_I(app, "ip ready");
}

static int32_t _wifi_event_handler(wifi_event_t event,
        uint8_t *payload,
        uint32_t length)
{
    struct netif *sta_if;

    LOG_I(app, "wifi event: %d", event);

    switch(event)
    {
    case WIFI_EVENT_IOT_INIT_COMPLETE:
        LOG_I(app, "wifi inited complete");
        break;
    case WIFI_EVENT_IOT_PORT_SECURE:
        sta_if = netif_find_by_type(NETIF_TYPE_STA);
        netif_set_status_callback(sta_if, _ip_ready_callback);
        netif_set_link_up(sta_if);
        dhcp_start(sta_if);

        uint8_t ssid[33] = {0};
        uint8_t ssid_len;
        wifi_config_get_ssid(0, ssid, &(ssid_len));
        LOG_I(app, "************************");
        LOG_I(app, "wifi connected to [%s]", ssid);
        LOG_I(app, "************************");

        _send_ble_indication(_WIFI_INFO_CONNECTED, NULL, 0);
        _send_ble_indication(_WIFI_INFO_SSID, ssid, ssid_len);

        break;
    case WIFI_EVENT_IOT_DISCONNECTED:
        sta_if = netif_find_by_type(NETIF_TYPE_STA);
        netif_set_link_down(sta_if);
        LOG_I(app, "wifi disconnected");

        _send_ble_indication(_WIFI_INFO_DISCONNECTED, NULL, 0);
        break;
    }

    return 1;
}

/* ---------------------------------------------------------------------------- */

/**
* @brief       Main function
* @param[in]   None.
* @return      None.
*/
int main(void)
{
    /* Do system initialization, eg: hardware, nvdm and random seed. */
    system_init();

    /* system log initialization.
     * This is the simplest way to initialize system log, that just inputs three NULLs
     * as input arguments. User can use advanved feature of system log along with NVDM.
     * For more details, please refer to the log dev guide under /doc folder or projects
     * under project/mtxxxx_hdk/apps/.
     */
    log_init(NULL, NULL, NULL);

    LOG_I(app, "main()");

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE , _wifi_event_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, _wifi_event_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_PORT_SECURE, _wifi_event_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, _wifi_event_handler);

    /* User initial the parameters for wifi initial process,  system will determin which wifi operation mode
     * will be started , and adopt which settings for the specific mode while wifi initial process is running*/
    wifi_config_t config = {0};
    config.opmode = WIFI_MODE_STA_ONLY;
    /*
    strcpy((char *)config.sta_config.ssid, (const char *)"MTK_STA");
    strcpy((char *)config.sta_config.password, (const char *)"12345678");
    config.sta_config.ssid_length = strlen((const char *)config.sta_config.ssid);
    config.sta_config.password_length = strlen((const char *)config.sta_config.password);
    */

    /* Initialize wifi stack and register wifi init complete event handler,
     * notes:  the wifi initial process will be implemented and finished while system task scheduler is running.*/
    wifi_init(&config, NULL);

    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization*/
    //lwip_network_init(config.opmode);
    //lwip_net_start(config.opmode);
    lwip_tcpip_config_t tcpip_config = {{0}, {0}, {0}, {0}, {0}, {0}};
    lwip_tcpip_init(&tcpip_config, WIFI_MODE_STA_ONLY);

    bt_create_task();
	
	/* As for generic HAL init APIs like: hal_uart_init(), hal_gpio_init() and hal_spi_master_init() etc,
     * user can call them when they need, which means user can call them here or in user task at runtime.
     */

    /* Create a user task for demo when and how to use wifi config API to change WiFI settings,
    Most WiFi APIs must be called in task scheduler, the system will work wrong if called in main(),
    For which API must be called in task, please refer to wifi_api.h or WiFi API reference.
    xTaskCreate(user_wifi_app_entry,
                UNIFY_USR_DEMO_TASK_NAME,
                UNIFY_USR_DEMO_TASK_STACKSIZE / 4,
                NULL, UNIFY_USR_DEMO_TASK_PRIO, NULL);
    user_wifi_app_entry is user's task entry function, which may be defined in another C file to do application job.
    UNIFY_USR_DEMO_TASK_NAME, UNIFY_USR_DEMO_TASK_STACKSIZE and UNIFY_USR_DEMO_TASK_PRIO should be defined
    in task_def.h. User needs to refer to example in task_def.h, then makes own task MACROs defined.
    */

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for( ;; );
}

