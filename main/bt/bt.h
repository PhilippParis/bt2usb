
#ifndef _BT_H_
#define _BT_H_

#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_hidh.h"
#include "esp_hid_gap.h"
#include "nvs_flash.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#define ESP_BD_ADDR_STR         "%02x:%02x:%02x:%02x:%02x:%02x"
#define ESP_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]
#else
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#endif

#define SCAN_DURATION_SECONDS 5

/**
 * @brief bluetooth devices connected callback
 */
typedef void (*bt_connect_callback_t)(uint8_t device_count);

/**
 * @brief BT init data struct
 */
typedef struct {
    uint8_t max_device_count;                       /*!< maximum number of HID devices supported; should be CONFIG_TINYUSB_HID_COUNT */
    bt_connect_callback_t bt_connect_callback;      /*!< devices-connected callback; returns the count of connected devices */
} bt_init_data_t;

/**
 * @brief scans for bluetooth hid devices; establishes connections with them and calls the specified callbacks
 * @param hidh_callback callback called when an BT event is received (e.g. input event)
 * @param gattc_callback  callback called when a GATT event is received (e.g. connection established)
 */
esp_err_t init_bluetooth(esp_event_handler_t hidh_callback, esp_gattc_cb_t gattc_callback, bt_init_data_t *init_data);


#endif //_BT_H_