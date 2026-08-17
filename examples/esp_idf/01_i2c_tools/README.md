# I2C Tools

[中文版本](./README_CN.md)

Interactive I2C command-line tools for scanning and debugging devices on an
I2C bus.

## Difficulty

Beginner to intermediate.

## Hardware Required

- One ESP32-P4 board.
- Optional I2C device connected to the configured SDA/SCL pins.
- Pull-up resistors if the board or module does not already provide them.

The example can still run without an external I2C device, but scans will not
find anything.

## Build and Flash

```bash
cd examples/esp_idf/01_i2c_tools
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In `menuconfig`, check **Example Configuration**:

- `SCL GPIO Num`
- `SDA GPIO Num`
- `Store command history in flash`

The I2C speed can be selected as 100 kHz or 400 kHz in
`menuconfig -> Example Configuration -> Default I2C speed`. After boot, the
example continuously transmits waveform test data (default address `0x50`,
data `0x55`). The runtime `i2cconfig --freq` command changes the speed and
restarts the waveform task.

Default ESP32-P4 pins:

| Signal | Default GPIO |
| --- | --- |
| SDA | GPIO7 |
| SCL | GPIO8 |

The initial bus setup enables internal pull-ups. External pull-ups or
module-provided pull-ups are recommended for better rising edges at 400 kHz.
The `i2cconfig` command can reconfigure the bus at runtime and keeps internal
pull-ups enabled for that runtime bus instance.

When command history is enabled, the example uses the custom FAT partition from
`partitions_example.csv` to save console history in flash.

## Common Commands

After the monitor opens, use the prompt:

```text
i2c-tools> i2cdetect
i2c-tools> i2cconfig --sda 7 --scl 8 --freq 100000
i2c-tools> i2cdetect             # observe the 100 kHz waveform
i2c-tools> i2cconfig --sda 7 --scl 8 --freq 400000
i2c-tools> i2cdetect             # observe the 400 kHz waveform
i2c-tools> i2cget -c 0x50 -r 0x00 -l 1
i2c-tools> i2cset -c 0x50 -r 0x00 0x12
```

For oscilloscope capture, connect SDA, SCL, and GND. The waveform task keeps
transmitting even without a slave device; address, data, and interval can be
adjusted in `menuconfig`.

Use `help` in the console to list supported commands and arguments.

## Troubleshooting

- If every address is empty, check SDA/SCL pins, power, ground, and pull-ups.
- If many addresses appear at once, check for wiring problems or missing
  pull-ups.
- If the target device is 5 V, confirm the I2C pins are level-shifted.
- If `i2cget` or `i2cset` fails after a scan succeeds, check whether the device
  expects 8-bit or 16-bit register addressing before reusing the command in an
  application.
