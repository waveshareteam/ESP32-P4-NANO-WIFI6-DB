# LVGL Demo v9

[English Version](./README.md)

在受支持的显示屏上运行 LVGL v9 demos。

## 难度

高级。

## 硬件要求

- ESP32-P4 开发板。
- 受支持的 LCD 面板。
- 已启用 PSRAM。

根据所选 LVGL demo 和开发板配置，可能需要触摸硬件。

## 构建和烧录

```bash
cd examples/esp_idf/07_lvgl_demo_v9
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

该示例使用 LVGL v9 和本地 `esp32_p4_nano_wifi6_db` BSP。修改 LVGL 或显示行为时，请检查 `main/idf_component.yml` 和 `sdkconfig.defaults`。

当前应用使用以下配置启动 BSP 显示：

- 旋转 `ESP_LV_ADAPTER_ROTATE_0`。
- 三缓冲局部防撕裂模式。
- 不交换或镜像触摸坐标轴。
- 默认启用 `lv_demo_benchmark()`。

其他 LVGL demos，例如 `lv_demo_music()` 和 `lv_demo_widgets()`，已经在 `sdkconfig.defaults` 中启用，可通过修改 `main/main.c` 中的调用来选择。

## 预期行为

显示屏应初始化、点亮背光，并运行带 LVGL 性能监视器的 benchmark demo。

## 排障

- 先运行 [06_Displaycolorbar](../06_Displaycolorbar/) 验证面板。
- 确认 PSRAM 已初始化。
- 检查 LVGL buffer 大小和显示分辨率设置。
- 如果触摸不工作，单独验证触摸控制器和 I2C 引脚。
- 如果修改防撕裂模式，请保持 `main/main.c` 中的 `tear_avoid_mode` 与
  `sdkconfig.defaults` 中的 `CONFIG_BSP_LCD_DPI_BUFFER_NUMS` 一致。
