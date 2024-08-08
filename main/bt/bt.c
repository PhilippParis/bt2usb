#include "bt.h"

#include <stdio.h>  
#include <string.h> 

static const char *TAG = "BLUETOOTH";

#if CONFIG_BT_NIMBLE_ENABLED
void ble_hid_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_store_config_init(void);
#endif

void hid_connect_task(void *pvParameters)
{
    size_t results_len = 0;
    esp_hid_scan_result_t *results = NULL;
    bt_init_data_t *init_data = (bt_init_data_t *) pvParameters;

    // start scan for HID devices
    ESP_LOGI(TAG, "SCAN...");
    esp_hid_scan(SCAN_DURATION_SECONDS, &results_len, &results);

    ESP_LOGI(TAG, "SCAN: %u results", results_len);
    if (results_len) {
        esp_hid_scan_result_t *r = results;
        for(uint8_t i = 0; i < init_data->max_device_count && i < results_len; i++) {
            printf("CONNECTING TO: %s \n", r->name ? r->name : "");
            esp_hidh_dev_open(r->bda, r->transport, r->ble.addr_type);
            r = r->next;
        }
        esp_hid_scan_results_free(results);
    }
    init_data->bt_connect_callback(results_len < init_data->max_device_count ? results_len : init_data->max_device_count);
    vTaskDelete(NULL);
}

esp_err_t init_bluetooth(esp_event_handler_t hidh_callback, esp_gattc_cb_t gattc_callback, bt_init_data_t *init_data)  {
#if HID_HOST_MODE == HIDH_IDLE_MODE
    ESP_LOGE(TAG, "Please turn on BT HID host or BLE!");
    return ESP_FAIL;
#endif
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "setting hid gap, mode:%d", HID_HOST_MODE);
    ESP_ERROR_CHECK( esp_hid_gap_init(HID_HOST_MODE) );

#if CONFIG_BT_BLE_ENABLED
    ESP_ERROR_CHECK( esp_ble_gattc_register_callback(gattc_callback) );
#endif /* CONFIG_BT_BLE_ENABLED */

    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK( esp_hidh_init(&config) );

    // TODO
    //ESP_LOGI(TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
#if CONFIG_BT_NIMBLE_ENABLED
    /* XXX Need to have template for store */
    ble_store_config_init();

    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    /* Starting nimble task after gatts is initialized*/
    ret = esp_nimble_enable(ble_hid_host_task);
    if (ret) {
        ESP_LOGE(TAG, "esp_nimble_enable failed: %d", ret);
    }
#endif
    xTaskCreate(&hid_connect_task, "hid_task", 6 * 1024, init_data, 2, NULL);
    return ESP_OK;
}