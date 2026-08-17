# I2S Codec

[中文版本](./README_CN.md)

Audio codec example using I2S. It can run in music playback mode or echo mode,
depending on configuration. For ESP32-P4, the default BSP is the local
`esp32_p4_nano_wifi6_db` component.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4-NANO-WIFI6-DB with its onboard ES8311 codec and single analog microphone.
- Speaker or headphones for playback mode.
- Microphone input for echo mode, if used.

## Build and Flash

```bash
cd examples/esp_idf/05_I2SCodec
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In **Example Configuration**, check:

- Mode: music or echo.
- Microphone gain.
- Voice volume.
- BSP support option. It is enabled by default for ESP32-P4 and selects
  `firmware/esp32_p4_nano_wifi6_db` from `main/idf_component.yml`.

ESP32-P4-NANO-WIFI6-DB audio wiring used by the BSP:

| Signal | Default GPIO |
| --- | --- |
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |
| I2S MCLK | GPIO13 |
| I2S BCLK | GPIO12 |
| I2S WS | GPIO10 |
| I2S DOUT | GPIO9 |
| I2S DIN | GPIO11 |
| Power amplifier enable | GPIO53 |

The BSP configures ES8311 in single analog microphone mode and owns the I2C,
I2S, and power-amplifier setup. Echo mode is supported by the BSP path as well;
the selected microphone gain is applied to the codec.

## Expected Behavior

- Music mode plays the bundled `canon.pcm` sample.
- Echo mode routes microphone input through the codec path.
- The log prints `Using ESP32-P4-NANO-WIFI6-DB BSP for I2S and codec configuration`,
  `i2s driver init success`, and `es8311 codec init success` before audio
  playback or echo starts.

## Troubleshooting

- Confirm codec power, clock, I2C control bus, and I2S pins.
- Start with a moderate volume.
- If there is no sound, check whether the selected board path matches your
  hardware.
- If codec initialization fails, run [01_i2c_tools](../01_i2c_tools/) on the
  same SDA/SCL pins to confirm the ES8311 responds on the control bus.
