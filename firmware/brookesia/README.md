# ESP-Brookesia Factory Firmware

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5%2B%20%7C%20v6.1-E7352C?logo=espressif)](https://github.com/espressif/esp-idf/releases)
[![Target](https://img.shields.io/badge/target-ESP32--P4-1F6FEB)](https://www.espressif.com/en/products/socs/esp32-p4)
[![LVGL](https://img.shields.io/badge/LVGL-9.4.0-2AA9E0?logo=lvgl)](https://lvgl.io/)

English | [简体中文](README_CN.md)

This directory contains the ESP-Brookesia firmware source for the
**Waveshare ESP32-P4-WIFI6-DB-POE-ETH**. It drives an external MIPI-DSI touch
panel, the ES8311 audio codec, the OV5647 camera, the SD card, ESP32-C5 hosted
Wi-Fi and IP101 Ethernet.

## Platform configuration

| Item | Configuration |
| --- | --- |
| Host MCU | ESP32-P4NRW32X, 32 MB flash, 32 MB PSRAM |
| Display / touch | Waveshare [5](https://www.waveshare.com/5-dsi-touch-a.htm), [7](https://www.waveshare.com/7-dsi-touch-a.htm), [8](https://www.waveshare.com/8-dsi-touch-a.htm) and [10.1](https://www.waveshare.com/10.1-dsi-touch-a.htm)-inch portrait panels, GT911 touch |
| Framework | ESP-IDF v6.1 (locked in `dependencies.lock`); `idf_component.yml` requires `>=5.5` |
| UI | LVGL 9.4.0 with the local ESP-Brookesia core 0.6.0-beta2 |
| BSP | Local `esp32_p4_wifi6_db_poe_eth` component in [`components/`](../../components/esp32_p4_wifi6_db_poe_eth/) |
| LVGL adapter | `espressif/esp_lvgl_adapter` 0.6.3 |
| Wireless | ESP32-C5 over SDIO using ESP-Hosted 3.0.10 / `esp_wifi_remote` 1.6.5 |
| Ethernet | IP101 RMII PHY through `espressif/ethernet_init` 1.4.1 |
| Voice assistant | `espressif/esp_xiaozhi` 0.1.1, ESP-SR 2.4.7 and xiaozhi-fonts 1.6.0 |
| Media | `espressif/esp_extractor` 1.0.7, `esp_new_jpeg` 1.0.2, `esp_video` 2.5.0 |

Select the panel under `menuconfig` → the BSP's LCD type option
(`CONFIG_BSP_LCD_TYPE_*`); the default is the 10.1-inch 800 x 1280 panel. See
the [hardware reference guide](../../docs/en/hw-reference/user-guide.md)
for interfaces and pin assignments. The ESP32-P4/ESP32-C5 companion project is
[`15_wifi_iperf`](../../examples/esp-idf/15_wifi_iperf/).

## Applications

<table>
  <tr>
    <td align="center"><img src="components/draw/assets/img_app_drawpanel.png" width="64" alt="DrawPanel icon"><br><b>DrawPanel</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline icon"><br><b>Squareline</b></td>
    <td align="center"><img src="components/brookesia_app_calculator/assets/img_app_calculator.png" width="64" alt="Calculator icon"><br><b>Calculator</b></td>
    <td align="center"><img src="components/SpecAnalyzer/assets/img_app_specanalyzer.png" width="64" alt="SpecAnalyzer icon"><br><b>SpecAnalyzer</b></td>
    <td align="center"><img src="components/MusicPlayer/assets/img_app_musicplayer.png" width="64" alt="MusicPlayer icon"><br><b>MusicPlayer</b></td>
  </tr>
  <tr>
    <td align="center"><img src="components/Camera/assets/img_app_camera.png" width="64" alt="Camera icon"><br><b>Camera</b></td>
    <td align="center"><img src="components/VideoPlayer/assets/img_app_vedioplayer.png" width="64" alt="VideoPlayer icon"><br><b>VideoPlayer</b></td>
    <td align="center"><img src="components/Settings/assets/img_app_settings.png" width="64" alt="Settings icon"><br><b>Settings</b></td>
    <td align="center"><img src="components/Ethernet/assets/img_app_ethernet.png" width="64" alt="Ethernet icon"><br><b>Ethernet</b></td>
    <td align="center"><img src="components/XiaozhiApp/assets/img_app_xiaozhi.png" width="64" alt="AIChats icon"><br><b>AIChats</b></td>
  </tr>
</table>

| Application | Function | Runtime dependency |
| --- | --- | --- |
| DrawPanel | Drawing in a fixed canvas area without dragging the entire app at the edge | Display and touch |
| Squareline | Registry-installed Squareline Studio UI demo and application template | Display and touch |
| Calculator | Touch-friendly Brookesia calculator | Display and touch |
| SpecAnalyzer | Single-channel FFT spectrum from the on-board microphone | ES8311 input |
| MusicPlayer | Plays the MP3 files in `/spiffs/music` with a spectrum visualizer | SPIFFS `storage`, ES8311 output |
| Camera | OV5647 MIPI CSI preview, scaled by the PPA to fill the panel | OV5647, `esp_video` |
| VideoPlayer | Full-screen SD-card AVI/MJPEG playback in the video's own orientation, with PCM audio | SD card, JPEG/PPA, ES8311 output |
| Settings | WLAN scan/connect, sound, display brightness, SNTP time and device information; brightness and volume are kept in NVS | ESP32-C5 hosted Wi-Fi |
| Ethernet | Link state, IPv4 address, netmask, gateway, MAC address and PHY of the IP101 port | IP101, DHCP server on the network |
| AIChats | Network check, Xiaozhi activation, WakeNet wake word, VAD, Opus uplink/downlink, server transcript and TTS | Wi-Fi or Ethernet, `model`/`storage` images, ES8311 |

## Build

### Requirements

Use ESP-IDF v6.1 (or v5.5 or later) with the `esp32p4` toolchain. Network access
to the ESP Component Registry is required. Use a data-capable USB cable for
flashing.

All commands below run from this directory. Starting from the repository root:

```sh
cd firmware/brookesia
idf.py --version
idf.py set-target esp32p4
idf.py build
```

### First full flash and partition changes

The first installation must write the bootloader, partition table, application,
ESP-SR model and SPIFFS resources:

```sh
idf.py -p PORT flash monitor
```

Erase the flash before a full flash when changing partition offsets or sizes,
upgrading from an older partition layout, or investigating stale model or
filesystem images:

```sh
idf.py -p PORT erase-flash flash monitor
```

When the partition layout, voice model and `spiffs/` resources are unchanged,
you can flash only the application:

```sh
idf.py -p PORT app-flash monitor
```

`app-flash` does not update `srmodels.bin`, `storage.bin` or the partition
table. Run a full `flash` after changing `partitions.csv`, WakeNet/model Kconfig,
xiaozhi-fonts, bundled music or prompt audio.

### Flash partitions

The layout is defined in `partitions.csv`; the partition table is at `0x8000`:

| Name | Type | Offset | Size | Purpose |
| --- | --- | --- | --- | --- |
| `nvsfactory` | data / nvs | `0x9000` | 200 KiB | Factory NVS data |
| `nvs` | data / nvs | `0x3B000` | 840 KiB | Wi-Fi, app settings and Xiaozhi state |
| `otadata` | data / ota | `0x10D000` | 8 KiB | OTA data (reserved) |
| `phy_init` | data / phy | `0x10F000` | 4 KiB | PHY calibration data |
| `model` | data / spiffs | `0x110000` | 960 KiB | ESP-SR speech models (`srmodels.bin`) |
| `factory` | app / factory | `0x200000` | 8 MiB | Application |
| `storage` | data / spiffs | `0xA00000` | 4 MiB | Music, Xiaozhi prompts and font (`storage.bin`) |

## AIChats / Xiaozhi

AIChats uses `esp_xiaozhi` for the protocol. This project provides the UI,
activation, audio and lifecycle integration.

First use:

1. Configure WLAN in Settings or connect Ethernet.
2. Open AIChats. If the device is not associated with an account, it displays
   and announces an activation code.
3. Add the device in the [Xiaozhi control panel](https://xiaozhi.me) and enter
   the activation code.
4. When the page reaches standby, say the wake phrase. The default is
   "Ni Hao Xiao Zhi."

If the log reports `No speech model found` or `contains no compatible models`,
rebuild and perform a full flash to write both the partition table and
`srmodels/srmodels.bin`.

The local UI language can be selected in this menu:

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language
      English
      Simplified Chinese
```

This option controls firmware-owned text only. It does not translate
conversation text, TTS or activation messages returned by the server. Local
strings are in `components/XiaozhiApp/XiaozhiLocalization.*`.

The board has one microphone on the ES8311. AIChats and SpecAnalyzer capture a
single channel; AIChats converts it to 16 kHz mono Opus for uplink, and the
playback chain converts server audio as needed for the codec output.

## Camera and media

Camera runs the OV5647 in its 800 x 640 binned mode. The PPA cover-fills the
panel with the frame, cropping the source so that the scale factor lands on the
PPA's 1/16 steps.

VideoPlayer scans only `/sdcard/avi`; the `.avi` extension is case-insensitive.
AVI files in the SD card root and other containers such as MP4 or MOV are not
listed. The player demuxes AVI with `esp_extractor`, decodes MJPEG with the
hardware JPEG decoder (falling back to `esp_new_jpeg` for frames the hardware
cannot decode, such as progressive JPEG), and writes frames through the PPA
directly into the LCD frame buffer:

- Only MJPEG video is played. Only PCM audio is played; other audio formats
  are skipped and the video plays silently.
- A frame whose orientation differs from the panel's is rotated a quarter turn,
  so landscape video fills the portrait panel sideways. If it comes out upside
  down, change `VIDEO_ROTATED_ANGLE` in `VideoPlayer.cpp` to
  `PPA_SRM_ROTATION_ANGLE_270`.
- Frames are cropped to fill the screen; a 16:9 video on a 16:10 panel loses
  about 5% on each side.

Convert other videos with ffmpeg, for example:

```sh
ffmpeg -i input.mov -c:v mjpeg -q:v 5 -pix_fmt yuvj420p -r 24 -c:a pcm_s16le -ar 44100 -ac 2 output.avi
```

Raise `-q:v` (lower quality) or scale the video down if playback drops frames.

MusicPlayer plays the MP3 files in `/spiffs/music`. After changing tracks,
regenerate and flash `storage.bin`; `app-flash` alone does not update it.

## Development notes

- Apps register themselves with `ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR`.
  `BOOT_APP_GROUPS` in `main/main.cpp` sets only the home-screen order and the
  boot stage shown while each group installs; registered apps not listed there
  are installed after the last group.
- New apps derive from `esp_brookesia::systems::phone::App` and implement
  `run()` and `back()`. Override other lifecycle functions according to
  resource ownership. Each component declares its own dependencies in its
  `idf_component.yml`; `main` keeps only the ones it uses directly.
- The MCP tool entry point is `registerXiaozhiDeveloperTools()` in
  `components/XiaozhiApp/XiaozhiDeveloperTools.cpp`. Register only implemented
  tools that return real results; run long operations in an application task.
- Access LVGL objects on the LVGL thread or while holding the display lock
  briefly. On app shutdown, release its timers, tasks, queues, codec sessions,
  SD handles and LVGL objects.
- During playback, VideoPlayer enables the adapter's Dummy Draw and pauses LVGL.
  On exit, it stops playback and resumes LVGL rendering.

## Notes

- Use the LVGL lock only for UI operations. Run networking, SD scans, audio
  initialization, NVS access and model loading outside the lock.
- Model, font, prompt, music and partition changes require a full `flash`.
- Audio apps share one physical bus. Stop or hand off the codec session when
  switching between apps.
- The host component must remain compatible with the coprocessor firmware on
  the ESP32-C5.

`EXAMPLE_SHOW_MEM_INFO` in `main/main.cpp` is currently `true`, so periodic
SRAM/PSRAM statistics are enabled. The detailed log output is controlled
separately by `EXAMPLE_SHOW_MEM_INFO_LOG`.

When reporting an issue, include the board revision, ESP-IDF version, complete
reproduction steps and the first actual error in the logs.

The local Brookesia core uses the
[Apache-2.0 license](components/brookesia_core/license.txt), which applies only
to that component.
