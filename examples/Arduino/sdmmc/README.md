# sdmmc

## 作用

使用 4-bit SDMMC 接口初始化板载 microSD 卡，打印卡片和文件系统容量，并对测试
文件进行读写。示例会自动打开卡电源，不需要额外控制 GPIO。

## 使用方法

1. 将 microSD 卡插入开发板。
2. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
3. 打开 `sdmmc.ino`，编译并上传。
4. 打开串口监视器，波特率设置为 `115200`。
5. 查看卡片信息以及 `/arduino_test.txt` 的写入和读取结果。

## 板级连接

- SDMMC CLK/CMD：GPIO43/GPIO44。
- SDMMC D0-D3：GPIO39/GPIO40/GPIO41/GPIO42。
- 卡电源控制：GPIO45，低电平打开。

## 预期输出

```text
Starting SDMMC test
SD card detected: ...
Card size: ... MB
Filesystem total: ... MB
Writing /arduino_test.txt
Reading /arduino_test.txt
ESP32-P4 SDMMC test
SDMMC test completed successfully
```

如果挂载失败，先确认卡已插入、格式可识别，并检查是否有其他示例占用了 SDMMC
引脚。测试文件位于卡的根目录，测试完成后可以手动删除。
