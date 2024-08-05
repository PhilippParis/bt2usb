#ifndef _USB_H_
#define _USB_H_

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
//#include "class/hid/hid_device.h"


#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)


esp_err_t init_usb();

# endif // _USB_H_