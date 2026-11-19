# ESP32-P4-NANO-WIFI6-DB Firmware

[English](README.md) | [简体中文](README_ZH.md)

The ESP32-P4 factory image below is for the 10.1-DSI-TOUCH-A display. It is a merged image flashed at `0x0`; the date in its name identifies the version.

| File | Purpose |
| --- | --- |
| [ESP32-P4-NANO-WIFI6-DB-10.1-DSI-TOUCH-A_Factory_20261031.bin](ESP32-P4-NANO-WIFI6-DB-10.1-DSI-TOUCH-A_Factory_20261031.bin) | Factory firmware for the 10.1-DSI-TOUCH-A display |

## Flashing

Run the commands from this directory. Replace `<PORT>` with the ESP32-P4 serial port. Erasing removes the existing data from ESP32-P4 flash.

```bash
python -m pip install esptool
python -m esptool --chip esp32p4 --port <PORT> erase-flash
python -m esptool --chip esp32p4 --port <PORT> write-flash 0x0 ESP32-P4-NANO-WIFI6-DB-10.1-DSI-TOUCH-A_Factory_20261031.bin
```

The ESP32-C5 wireless coprocessor firmware is available as [esp32-c5-slave-260623.bin](ESP32-C5/esp32-c5-slave-260623.bin). See [brookesia/README.md](brookesia/README.md) for the ESP-Brookesia example.
