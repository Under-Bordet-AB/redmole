/*
 * SPDX-FileCopyrightText: 2021-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


/****************************************************************************
* This is a demo for bluetooth config wifi connection to ap. You can config ESP32 to connect a softap
* or config ESP32 as a softap to be connected by other device. APP can be downloaded from github
* android source code: https://github.com/EspressifApp/EspBlufi
* iOS source code: https://github.com/EspressifApp/EspBlufiForiOS
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

#include "nvs_flash.h"

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
#include "esp_bt.h"
#endif

#include "esp_blufi_api.h"
#include "blufi_example.h"

#include "esp_log.h"
#include "esp_blufi.h"
#include "nac.h"

#ifndef CONFIG_SOC_BLUFI_SUPPORTED
#error "This SOC does not support BLUFI"
#endif

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);

static wifi_config_t sta_config;

// Header vars
bool ble_is_connected = false;
bool ble_is_initialized = false;

static esp_blufi_callbacks_t example_callbacks = {
    .event_cb = example_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        BLUFI_INFO("BLUFI init finish\n");
        ble_is_initialized = true;
#if SOC_MPI_SUPPORTED
        esp_blufi_adv_start();
#endif
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        BLUFI_INFO("BLUFI deinit finish\n");
        ble_is_initialized = false;
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        BLUFI_INFO("BLUFI ble connect\n");
        ble_is_connected = true;
        esp_blufi_adv_stop();
        blufi_security_init();
        #ifdef CONFIG_EXAMPLE_BLUFI_BLE_SMP_ENABLE
        // Try to initiate BLE security request after connection established.
        BLUFI_INFO("Try to initiate BLE security request\n");
        esp_err_t ret = esp_blufi_start_security_request(param->connect.remote_bda);
        if (ret != ESP_OK) {
            BLUFI_ERROR("Failed to start security request: %s\n", esp_err_to_name(ret));
        }
        #endif // CONFIG_EXAMPLE_BLUFI_BLE_SMP_ENABLE
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        BLUFI_INFO("BLUFI ble disconnect\n");
        ble_is_connected = false;
        blufi_security_deinit();
#if !SOC_MPI_SUPPORTED
        blufi_dh_pregen_start_with_cb(esp_blufi_adv_start);
#else
        esp_blufi_adv_start();
#endif
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        BLUFI_INFO("BLUFI request wifi connect to AP\n");
        nac_wifi_status_t nac_status = nac_get_wifi_status();
        if(nac_status != NAC_WIFI_DISCONNECTED)
        {
            esp_blufi_send_error_info(ESP_BLUFI_SEQUENCE_ERROR);
            break;
        }
        nac_request_wifi_connect((char*)sta_config.sta.ssid, (char*)sta_config.sta.password);
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        BLUFI_INFO("BLUFI request wifi disconnect from AP\n");
        nac_request_wifi_disconnect();
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        BLUFI_ERROR("BLUFI report error, error code %d\n", param->report_error.state);
        esp_blufi_send_error_info(param->report_error.state);
        break;
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        BLUFI_INFO("blufi close a gatt connection");
        ble_is_connected = false;
        esp_blufi_disconnect();
        break;
	case ESP_BLUFI_EVENT_RECV_STA_SSID:
        if (param->sta_ssid.ssid_len >= sizeof(sta_config.sta.ssid)/sizeof(sta_config.sta.ssid[0])) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            BLUFI_INFO("Invalid STA SSID\n");
            break;
        }
        strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA SSID %s\n", sta_config.sta.ssid);
        break;
	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        if (param->sta_passwd.passwd_len >= sizeof(sta_config.sta.password)/sizeof(sta_config.sta.password[0])) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            BLUFI_INFO("Invalid STA PASSWORD\n");
            break;
        }
        strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA PASSWORD %s\n", sta_config.sta.password);
        break;
    default:
        esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
        break;
    }
}

void blufi_main(void)
{
    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
    ret = esp_blufi_controller_init();
    if (ret) {
        BLUFI_ERROR("%s BLUFI controller init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
#endif

    ret = esp_blufi_host_and_cb_init(&example_callbacks);
    if (ret) {
        BLUFI_ERROR("%s initialise failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if !SOC_MPI_SUPPORTED
    blufi_dh_pregen_start();
    blufi_dh_pregen_wait();
    esp_blufi_adv_start();
#endif

    BLUFI_INFO("BLUFI VERSION %04x\n", esp_blufi_get_version());
}