# ES8311 I2S loopback

这个示例适用于 `ESP32-P4-NANO-WIFI6-DB`，使用板载 ES8311 将单路模拟麦克风
以单声道采样后回放到单个扬声器。

## 使用方法

1. 使用 Arduino-ESP32 `3.3.11`，选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
2. 使用板载麦克风和扬声器；如果连接外部音频器件，必须确认其电平和接口与 ES8311
   一致。
3. 打开 `115200` 波特率串口监视器，上传后示例以 48000 Hz、16-bit、I2S 标准
   单声道格式运行连续回环。

## 板级参数

- ES8311 控制地址：Arduino `Wire1` 的 7-bit 地址 `0x18`（ESP-IDF BSP
  使用的 8-bit 写地址为 `0x30`）。
- I2C1：SDA GPIO7，SCL GPIO8。
- I2S：MCLK GPIO13、BCLK GPIO12、LRCK GPIO10、DOUT GPIO9、DIN GPIO11，使用
  单声道 slot（左 slot）。
- 功放使能：GPIO53，高电平打开。
- 麦克风 ADC 增益：本示例通过本地 ES8311 控制助手设置为 24 dB；这不是 Nano BSP 的默认录音增益声明。
- MCLK 为 12.288 MHz（48 kHz × 256）；GPIO13 必须接入 ES8311 MCLK。

## 预期输出和注意事项

启动成功后会看到 `ES8311 ready`，随后每秒输出一次 `I2S input peak`。如果 I2C
扫描不到 `0x18`、MCLK 未连接或功放没有打开，回环不会正常工作。麦克风和扬声器
距离过近时会产生声学啸叫，请先降低音量或分开放置。

示例中的 ES8311 寄存器初始化来自 Espressif `esp_codec_dev` 的 ES8311
驱动配置；Arduino 核心没有直接提供 `esp_codec_dev` 的板级 BSP 封装，
因此这里保留了一个只覆盖当前 48 kHz 回环路径的本地控制助手。
