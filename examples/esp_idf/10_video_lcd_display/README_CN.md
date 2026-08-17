| 支持目标 | ESP32-P4 |
| -------- | -------- |

[English Version](./README.md)

# Video LCD Display

该示例基于 [esp_video](https://github.com/espressif/esp-video-components/tree/master/esp_video) 组件，演示如何把摄像头图像显示到 LCD 屏幕上。应用会初始化 ESP32-P4-NANO-WIFI6-DB Nano BSP 显示，打开 MIPI-CSI video device，使用 PPA 进行缩放/旋转/镜像处理，通过 BSP 的 framebuffer ownership 接口获取空闲 LCD framebuffer，并通过 LVGL adapter 显示流水线提交完整帧。

## ESP-IDF 要求

- 该示例要求 ESP-IDF 5.5 或更高版本，因为 Nano BSP 的最低版本要求为 5.5。
- 工程依赖 `esp_video` 和通过 `main/idf_component.yml` 选择的本地 `esp32_p4_nano_wifi6_db` BSP。
- 请按照 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html) 设置开发环境。**我们强烈建议**先 [构建第一个工程](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#build-your-first-project)，熟悉 ESP-IDF 并确保环境正确。

### 前置条件

* 一个 ESP32-P4-NANO-WIFI6-DB 开发板。
* 一个由 Nano BSP 配置的受支持 Waveshare MIPI-DSI 面板。当前工程配置选择 8 英寸 800 x 1280 面板；需要时可在 menuconfig 中选择其他面板。
* 一个受 `esp_video` 支持的 MIPI-CSI 摄像头传感器。默认 `sdkconfig.defaults` 选择 OV5647，MIPI RAW8 `800 x 1280`，50 FPS。
* 用于供电和烧录的 USB-C 线。
* 通过开发板的 `MIPI_DSI` 接口连接 LCD，通过 `MIPI_CSI` 接口连接摄像头。
* 按照开发板文档连接用于供电、烧录和查看串口输出的 USB-C 线。

### 配置工程

运行 `idf.py menuconfig`，配置 BSP 显示、摄像头传感器和视频流水线选项。

MIPI-CSI 摄像头默认 ESP32-P4 SCCB/I2C 引脚：

| 信号 | 默认 GPIO |
| --- | --- |
| SCL | GPIO8 |
| SDA | GPIO7 |

在 `Espressif Camera Sensors` 配置菜单中，选择与你的硬件匹配的摄像头传感器。当前默认值为：

```text
Component config  --->
    Espressif Camera Sensors Configurations  --->
        [*] OV5647  ---->
            Default format select for MIPI  --->
                (X) RAW8 800x1280 50fps, MIPI input
```

如果使用 SC2336 或其他传感器，请把传感器选择和输出格式改成与摄像头模块匹配。

### 构建和烧录

构建工程并烧录到开发板，然后运行监视工具查看串口输出（将 `PORT` 替换为开发板串口名）：

```bash
idf.py set-target esp32p4
idf.py -p PORT flash monitor
```

输入 `Ctrl-]` 退出串口监视器。

完整配置和使用 ESP-IDF 构建工程的步骤，请参见 [ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/get-started/index.html)。

### 预期行为

显示背光点亮，LCD 显示实时摄像头图像。日志会打印 video driver 版本、设备名、总线信息，以及检测到的帧宽高。摄像头帧分配在 PSRAM 中。每帧先获取空闲 LCD framebuffer，使用 PPA 写入完整的显示尺寸，然后通过 `bsp_display_flush_frame_buffer()` 提交。LCD 使用 `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3` 配置的三重 framebuffer。

### 显示 Framebuffer 所有权流程

摄像头到 LCD 的显示路径遵循以下 framebuffer ownership 流程：

1. 调用 `bsp_display_get_free_frame_buffer()` 获取一个可写的 LCD framebuffer。
2. 使用 PPA 把完整的显示尺寸帧写入该 buffer。如果摄像头图像没有覆盖整个 LCD，需要在 PPA 处理前清空目标 framebuffer，确保未覆盖区域内容确定。
3. 调用 `bsp_display_flush_frame_buffer(frame_buffer)` 把该 buffer 提交给显示流水线。提交后，在再次通过 `bsp_display_get_free_frame_buffer()` 获取之前，不要继续读写这个 buffer。

这样可以保证摄像头图像高度小于 LCD 高度时，整帧内容保持一致，并避免启用三重缓冲时未覆盖区域出现残留内容或闪烁。该路径不要直接索引 LCD framebuffer，也不要使用 `esp_lcd_dpi_panel_get_frame_buffer()` / `esp_lv_adapter_dummy_draw_blit()`。

### 排障

- 先运行 [06_Displaycolorbar](../06_Displaycolorbar/) 验证 LCD 路径。
- 如果出现 `video cam open failed`，检查摄像头 FPC 方向、传感器供电、SCCB/I2C 引脚和已选传感器型号。
- 确认 PSRAM 已启用且稳定；摄像头 buffer 会从 PSRAM 分配。
- 如果图像裁剪、镜像或旋转不正确，请调整传感器输出格式或 `main/main.c` 中的 PPA 操作。
