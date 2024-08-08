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

static const char *devices[] = {"MX Master 3", "M720 Triathlon"};
// set CONFIG_TINYUSB_HID_COUNT to number of devices!

hid_device_t *hid_devices = NULL;
uint8_t hid_device_count = 0;

bool compare_arrays(uint8_t *arr1, uint8_t *arr2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (arr1[i] != arr2[i]) {
            return false;
        }
    }
    return true;
}

int get_hid_device_index(uint8_t *bda) {    
    for(int i = 0; i < hid_device_count; i++) {
        printf("get_hid_device_index: BDA: %02x:%02x:%02x:%02x:%02x:%02x \n", ESP_BD_ADDR_HEX(hid_devices[i].bda));
        if (compare_arrays(hid_devices[i].bda, bda, 6)) {
            return i;
        }
    }
    return -1;
}

void bt_gattc_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    esp_hidh_gattc_event_handler(event, gattc_if, param);
    switch (event) {
        case ESP_GATTC_READ_CHAR_EVT:
            if (param->read.status == ESP_GATT_OK && param->read.value_len > 16) { // TODO check first bytes instead of length?
                hid_devices[hid_device_count].report_descriptor = malloc(param->read.value_len);
                memcpy(hid_devices[hid_device_count].report_descriptor, param->read.value, param->read.value_len);
                hid_devices[hid_device_count].report_len = param->read.value_len;
                hid_device_count++;

                printf("READ: CONNECTION ID: %i \n", param->read.conn_id);
                printf("READ: HANDLE: %i \n", param->read.handle);
                // TODO add BDA to hid_devices
                
                /*
                printf("HID Report Descriptor: ");
                for (int i = 0; i < param->read.value_len; i++) {
                    printf("%02x ", report_descriptor[i]);
                }
                printf("\n");
                */
            }
            break;
            case ESP_GATTC_CONNECT_EVT:
                printf("CONNECT: BDA: %02x:%02x:%02x:%02x:%02x:%02x \n", ESP_BD_ADDR_HEX(param->connect.remote_bda));
                printf("CONNECT: CONNECTION ID: %i \n", param->connect.conn_id);
                memcpy(hid_devices[hid_device_count].bda, param->connect.remote_bda, 6);
            break;

        default:
        break;
    }
}

void bt_hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (!tud_connected()) {
        return;
    }
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
            int instance = get_hid_device_index(esp_hidh_dev_bda_get(param->input.dev));
            printf("instance: %i\n", instance);
            if (instance >= 0) {
                tud_hid_n_report(instance, param->input.report_id, param->input.data, param->input.length);
            }
            break;
        case ESP_HIDH_FEATURE_EVENT:
            ESP_LOGI(TAG, "FEATURE");
            tud_hid_n_report(0, param->feature.report_id, param->feature.data, param->feature.length);
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
    bt_devices->names = (char **) devices;
    bt_devices->length = sizeof(devices) / sizeof(devices[0]);

    hid_devices = malloc(bt_devices->length * sizeof(hid_device_t *));

    // INIT BLUETOOTH
    init_bluetooth(bt_hidh_callback, bt_gattc_callback, bt_devices);
    // wait for BT devices to connect
    while(hid_device_count != bt_devices->length) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // INIT_USB
    init_usb(hid_devices, hid_device_count);

    // application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
