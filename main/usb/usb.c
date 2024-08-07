#include "usb.h"

static const char *TAG = "USB";

uint8_t *hid_report_descriptor;
uint8_t *hid_configuration_descriptor;

const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
};

/********* TinyUSB HID callbacks ***************/


// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    ESP_LOGI(TAG, "send hid report descriptors");
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

esp_err_t init_usb(uint8_t report_descriptor[], int report_len)
{
    ESP_LOGI(TAG, "USB initialization");
    // INIT HID REPORT DESCRIPTOR
    hid_report_descriptor = report_descriptor;
    
    // INIT HID CONFIGURATION DESCRIPTOR
    uint8_t configuration_descriptor[] = {
        // Configuration number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR(0, 4, false, report_len, 0x81, 16, 10),
    };
    hid_configuration_descriptor = malloc(sizeof(configuration_descriptor));
    memcpy(hid_configuration_descriptor, configuration_descriptor, sizeof(configuration_descriptor));
    
    // INIT USB CONFIG
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL, // TODO add device descriptor
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    return ESP_OK;
}