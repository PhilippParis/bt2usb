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

hid_device_t *hid_devices = NULL;
uint8_t hid_device_count = 0;
uint8_t expected_device_count = -1;


bool compare_arrays(uint8_t *arr1, uint8_t *arr2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (arr1[i] != arr2[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief returns the index of the device with the provided bluetooth address in the hid_devices array
 */
int get_hid_device_index(uint8_t *bda) {    
    for(int i = 0; i < hid_device_count; i++) {
        if (compare_arrays(hid_devices[i].bda, bda, 6)) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief BT GATTC callback; called when a BT connection to a device is establised
 */
void bt_gattc_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    esp_hidh_gattc_event_handler(event, gattc_if, param);
    switch (event) {
        case ESP_GATTC_CONNECT_EVT:
            // called first: save bluetooth address of device
            memcpy(hid_devices[hid_device_count].bda, param->connect.remote_bda, 6);
            break;
        case ESP_GATTC_READ_CHAR_EVT:
            // called second: save HID report descriptors
            if (param->read.status == ESP_GATT_OK && param->read.value_len > 16) { // TODO check first bytes instead of length?
                hid_devices[hid_device_count].report_descriptor = malloc(param->read.value_len);
                memcpy(hid_devices[hid_device_count].report_descriptor, param->read.value, param->read.value_len);
                hid_devices[hid_device_count].report_len = param->read.value_len;
                hid_device_count++;
            }
            break;
        default:
            break;
    }
}

/**
 * @brief BT HID callback; called when a BT event was received (e.g. input) and forwards it to USB
 */
void bt_hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (!tud_connected()) {
        return;
    }
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;
    int8_t instance = -1;

    // FORWARD BT HID EVENT TO USB
    switch (event) {
        case ESP_HIDH_INPUT_EVENT:
            instance = get_hid_device_index((uint8_t *) esp_hidh_dev_bda_get(param->input.dev));
            if (instance >= 0) {
                tud_hid_n_report(instance, param->input.report_id, param->input.data, param->input.length);
            }
            break;
        case ESP_HIDH_FEATURE_EVENT:
            instance = get_hid_device_index((uint8_t *) esp_hidh_dev_bda_get(param->feature.dev));
            if (instance >= 0) {
                tud_hid_n_report(instance, param->feature.report_id, param->feature.data, param->feature.length);
            }
            break;
        default:
            ESP_LOGI(TAG, "unknown BT event received");
        break;
    }
}

/**
 * @brief BT connect callback; called when all discovered devices are connected; returns the device count;
 */
void bt_connect_callback(uint8_t device_count) {
    expected_device_count = device_count;
}

/**
 * @brief main app loop
 */
void app_main(void)
{
    hid_devices = malloc(CONFIG_TINYUSB_HID_COUNT * sizeof(hid_device_t *));
    bt_init_data_t *bt_init_data = malloc(sizeof(bt_init_data_t));
    bt_init_data->max_device_count = CONFIG_TINYUSB_HID_COUNT;
    bt_init_data->bt_connect_callback = bt_connect_callback;

    // INIT BLUETOOTH
    init_bluetooth(bt_hidh_callback, bt_gattc_callback, bt_init_data);
    // wait for BT devices to connect
    while(expected_device_count != hid_device_count) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // INIT_USB
    init_usb(hid_devices, hid_device_count);

    // application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
