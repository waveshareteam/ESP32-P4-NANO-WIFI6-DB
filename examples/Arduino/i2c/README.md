# i2c

## 作用

扫描开发板默认 I2C1 总线上的设备，并打印检测到的 7 位 I2C 地址。本开发板默认
使用 SDA GPIO7、SCL GPIO8，频率为 400 kHz。扫描程序只把收到 ACK 的地址列为设备，
没有设备响应不代表程序异常。

## 使用方法

1. 将需要检测的 I2C 设备连接到开发板的 I2C 总线。
2. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
3. 打开 `i2c.ino`，编译并上传。
4. 打开串口监视器，波特率设置为 `115200`。
5. 查看扫描结果；程序每 5 秒自动重新扫描一次。

## 注意事项

- 该总线由 ES8311、LCD 背光、GT911 和摄像头 SCCB 等板载设备共享，扫描时不要同时
  运行其他会初始化 I2C1 的示例。
- 输出地址是 Arduino `Wire1` 使用的 7 位地址，不是包含读写位的 8 位控制地址。

## 预期输出

```text
Scanning I2C bus...
Device found at 0x...
Scan complete: ... device(s) found
```
