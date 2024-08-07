/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

#include "bt/bt.h"
#include "usb/usb.h"

static const char *TAG = "example";

static const char *devices[] = {"MX Master 3"};

uint8_t *report_descriptor = NULL;
uint8_t report_len = 0;

void bt_gattc_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    esp_hidh_gattc_event_handler(event, gattc_if, param);
    switch (event) {
        case ESP_GATTC_READ_CHAR_EVT:
            if (param->read.status == ESP_GATT_OK && param->read.value_len > 16) { // TODO check first bytesm instead of length?
                report_descriptor = malloc(param->read.value_len);
                memcpy(report_descriptor, param->read.value, param->read.value_len);
                report_len = param->read.value_len;
                /*
                printf("HID Report Descriptor: ");
                for (int i = 0; i < param->read.value_len; i++) {
                    printf("%02x ", report_descriptor[i]);
                }
                printf("\n");
                */
            }
            break;
        default:
        break;
    }
}

void bt_hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    // FORWARD BT HID EVENT TO USB
    switch (event) {
        case ESP_HIDH_OPEN_EVENT:
            ESP_LOGI(TAG, "OPEN");
            break;
        case ESP_HIDH_CLOSE_EVENT:
            ESP_LOGI(TAG, "CLOSE");
            break;
        case ESP_HIDH_INPUT_EVENT:
            //ESP_LOGI(TAG, "INPUT: %8s, MAP: %2u, ID: %3u, Len: %d, Data:", esp_hid_usage_str(param->input.usage), param->input.map_index, param->input.report_id, param->input.length);
            //ESP_LOG_BUFFER_HEX(TAG, param->input.data, param->input.length);
            tud_hid_report(param->input.report_id, param->input.data, param->input.length);
            break;
        case ESP_HIDH_FEATURE_EVENT:
            ESP_LOGI(TAG, "FEATURE");
            tud_hid_report(param->feature.report_id, param->feature.data, param->feature.length);
            break;
        case ESP_HIDH_BATTERY_EVENT:
            ESP_LOGI(TAG, "BATTERY");
            break;
        default:
            ESP_LOGI(TAG, "BT event received");
        break;
    }
}

void app_main(void)
{
    bt_device_names_t *bt_devices = malloc(sizeof(bt_device_names_t));
    bt_devices->names = devices;
    bt_devices->length = sizeof(devices) / sizeof(devices[0]);

    // INIT BLUETOOTH
    init_bluetooth(bt_hidh_callback, bt_gattc_callback, bt_devices);
    // wait for BT devices to connect
    while(report_descriptor == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // INIT_USB
    init_usb(report_descriptor, report_len);

    // application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// 1. mount USB
// 2. scan for BT devices
// 3. connect to mouse/keyboard -> register callback
// 4. forward BT-HID-host events to USB-host

