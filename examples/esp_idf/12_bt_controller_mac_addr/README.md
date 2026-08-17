# BT Controller MAC Address

[中文版本](./README_CN.md)

Verify ESP32-P4 Bluetooth controller access through ESP-Hosted.

This example connects the ESP32-P4 host to its wireless co-processor, prints
the co-processor firmware and chip information, reads the BT controller MAC
address, optionally updates that MAC address before controller initialization,
and starts a connectable BLE GATT server named `P4_GATTS_DEMO`.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board configured for ESP-Hosted wireless co-processor access.
- A compatible ESP32-series co-processor firmware with Bluetooth controller
  support enabled.
- A BLE scanner application for validation, such as nRF Connect.

ESP32-P4 does not provide native Bluetooth radio support by itself. The
Bluetooth controller in this example runs on the co-processor and the ESP32-P4
runs the Bluedroid host over Hosted VHCI.

## Build and Flash

Use an ESP-IDF terminal first. If an editor plugin fails to build this example,
verify the command-line flow before debugging the editor setup.

```bash
cd examples/esp_idf/12_bt_controller_mac_addr
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In `menuconfig`, open **Example Configuration**:

- Keep **Update BT Controller MAC address before init** disabled to only read
  the current BT MAC address.
- Enable it to temporarily set the address from **BT Controller MAC address**.
  The configured value must use `XX:XX:XX:XX:XX:XX` format.

The MAC update must happen before `esp_hosted_bt_controller_init()`. The change
is temporary and reverts after reset.

## Expected Output

The serial log should show:

- ESP-Hosted co-processor connection.
- Co-processor firmware and chip information.
- Current BT controller MAC address.
- Hosted BT controller initialization.
- `BLE advertising started as P4_GATTS_DEMO`.
- `GATTS app registered`.
- `GATT service 0x00FF started, char 0xFF01`.

After advertising starts, a BLE scanner should show `P4_GATTS_DEMO`.

## BLE Interaction

Use nRF Connect or another BLE scanner:

1. Scan for and connect to `P4_GATTS_DEMO`.
2. Discover service `0x00FF`.
3. Read characteristic `0xFF01`; the default value is `hello from esp32-p4`.
4. Write a short value to characteristic `0xFF01`.
5. Enable notifications on the same characteristic, then write again. The device
   echoes the written value back as a notification.
6. Disconnect. The device restarts advertising automatically.

On Windows, use **Bluetooth LE Explorer** or another BLE GATT client instead of
the system Bluetooth device settings page. Bluetooth LE Explorer may start a
pairing flow before GATT discovery; this example accepts Just Works bonding. If
pairing previously failed, remove the stale `P4_GATTS_DEMO` device from Windows
Bluetooth settings and scan again.

## Troubleshooting

- Confirm the co-processor firmware has ESP-Hosted Bluetooth support enabled.
- Confirm the ESP-Hosted transport pins and reset GPIO match the board.
- If the BT MAC read fails, check that the co-processor reports Bluetooth
  capability during ESP-Hosted startup.
- If advertising does not start, check that `CONFIG_BT_ENABLED`,
  `CONFIG_BT_CONTROLLER_DISABLED`, and Hosted Bluedroid VHCI are enabled in
  `sdkconfig`.
- If the device is visible but cannot connect, confirm the scanner is using the
  connectable `P4_GATTS_DEMO` advertising set and not a stale cached result.
- If Windows reports pairing failure, remove the device from Windows Bluetooth
  settings, restart the example, and retry from Bluetooth LE Explorer.
