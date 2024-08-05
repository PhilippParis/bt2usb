set(srcs "main.c"
         "bt/esp_hid_gap.c"
         "bt/bt.c"
         "usb/usb.c")
set(include_dirs ".")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES esp_hid
                       PRIV_REQUIRES esp_driver_gpio nvs_flash)
