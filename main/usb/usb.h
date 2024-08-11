#ifndef _USB_H_
#define _USB_H_

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID device information
 */
typedef struct {
    uint8_t *report_descriptor;         /*!< holds the HID report descriptors of this device */
    uint8_t report_len;                 /*!< count of report descriptors in report_descriptor array (one device can hold multiple descriptors) */
    uint8_t bda[6];                     /*!< bluetooth device address */
} hid_device_t;

/**
 * @brief sets up the USB connection using the HID report descriptors in 'devices'
 * @param devices array of hid device data
 * @param device_count devices array length
 */
esp_err_t init_usb(hid_device_t* devices, uint8_t device_count);

# endif // _USB_H_