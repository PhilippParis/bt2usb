/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "esp_event.h"
//#include "esp_hidh.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"

#include "bt/bt.h"
#include "usb/usb.h"


#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
static const char *TAG = "example";


/********* Application ***************/

typedef enum {
    MOUSE_DIR_RIGHT,
    MOUSE_DIR_DOWN,
    MOUSE_DIR_LEFT,
    MOUSE_DIR_UP,
    MOUSE_DIR_MAX,
} mouse_dir_t;

#define DISTANCE_MAX        125
#define DELTA_SCALAR        5


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

static void mouse_draw_square_next_delta(int8_t *delta_x_ret, int8_t *delta_y_ret)
{
    static mouse_dir_t cur_dir = MOUSE_DIR_RIGHT;
    static uint32_t distance = 0;

    // Calculate next delta
    if (cur_dir == MOUSE_DIR_RIGHT) {
        *delta_x_ret = DELTA_SCALAR;
        *delta_y_ret = 0;
    } else if (cur_dir == MOUSE_DIR_DOWN) {
        *delta_x_ret = 0;
        *delta_y_ret = DELTA_SCALAR;
    } else if (cur_dir == MOUSE_DIR_LEFT) {
        *delta_x_ret = -DELTA_SCALAR;
        *delta_y_ret = 0;
    } else if (cur_dir == MOUSE_DIR_UP) {
        *delta_x_ret = 0;
        *delta_y_ret = -DELTA_SCALAR;
    }

    // Update cumulative distance for current direction
    distance += DELTA_SCALAR;
    // Check if we need to change direction
    if (distance >= DISTANCE_MAX) {
        distance = 0;
        cur_dir++;
        if (cur_dir == MOUSE_DIR_MAX) {
            cur_dir = 0;
        }
    }
}

static void app_send_hid_demo(void)
{
    // Keyboard output: Send key 'a/A' pressed and released
    ESP_LOGI(TAG, "Sending Keyboard report");
    uint8_t keycode[6] = {HID_KEY_A};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);

    // Mouse output: Move mouse cursor in square trajectory
    ESP_LOGI(TAG, "Sending Mouse report");
    int8_t delta_x;
    int8_t delta_y;
    for (int i = 0; i < (DISTANCE_MAX / DELTA_SCALAR) * 4; i++) {
        // Get the next x and y delta in the draw square pattern
        mouse_draw_square_next_delta(&delta_x, &delta_y);
        tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, 0x00, delta_x, delta_y, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void init_gpio(void) {
    // Initialize debug button
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = true,
        .pull_down_en = false,
    };
    ESP_ERROR_CHECK(gpio_config(&boot_button_config));
}

void app_main(void)
{
    init_gpio();
    init_bluetooth(bt_hidh_callback, bt_gattc_callback);

    // wait for BT connection
    while(report_descriptor == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    init_usb(report_descriptor, report_len);

    while (1) {
        if (tud_mounted()) {
            static bool send_hid_data = true;
            if (send_hid_data) {
                app_send_hid_demo();
            }
            send_hid_data = !gpio_get_level(APP_BUTTON);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// 1. mount USB
// 2. scan for BT devices
// 3. connect to mouse/keyboard -> register callback
// 4. forward BT-HID-host events to USB-host

