# Bluetooth 2 USB HID Proxy

[PC] <---USB---> [ESP32-S3] <---BT-LE---> [Keyboard/Mouse]

Bluetooth to USB proxy for HID devices like mouse/keyboard to solve the following problems:
* using BT keyboard for Bitlocker password entry
* using BT keyboard to control KVM switch (e.g. switch display via keyboard shortcut)
* using BT keyboard for bootloader/BIOS


### Hardware Required

Any ESP board that has USB-OTG and Bluetooth support. e.g. ESP32-S3.

> [!NOTE]
> ESP32-S3 only supports Bluetooth-LE. Not Bluetooth Classic! make sure your devices support Bluetooth LE\
> if your device requires Bluetooth Classic use a raspberry pi zero W with https://github.com/quaxalber/bluetooth_2_usb

### Usage

connect the ESP to your PC via the USB-OTG port.\
After startup the application is scanning for bluetooth hid devices for SCAN_DURATION_SECONDS (default: 5) seconds.\
It connects automatically to all devices it finds during that scan. (at most CONFIG_TINYUSB_HID_COUNT number of devices; default: 4)


### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```bash
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)
