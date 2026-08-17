## USB 扩展屏示例

请在本仓库的示例目录中构建该适配版本；`main/idf_component.yml` 已声明本地 Nano BSP 覆盖路径。

USB 扩展屏示例可以将 ESP32-P4-NANO-WIFI6-DB 开发板作为 Windows 的副屏。支持以下功能。

* 支持最大 1024*576@60FPS 的横屏 USB 输入

* 将横屏输入旋转后显示到 Nano 原生的竖向 MIPI 屏幕，并居中显示

* 多出的面板区域保持黑屏；默认 800*1280 面板的有效内容为 576*1024

* 支持最多五点的屏幕触摸

* 支持音频的输入和输出

## 所需硬件

1. ESP32-P4-NANO-WIFI6-DB 开发板
2. Nano BSP 支持的 Waveshare MIPI-DSI 触摸屏
3. 启用 USB 音频输出时连接扬声器

## 硬件连接

* 连接

    1. 将开发板上的高速 USB 口连接到 PC 上

## 编译和烧录

### 设备端

构建项目并将其烧录到板子上，然后运行监控工具以查看串行输出：

* 运行 `. ./export.sh` 以设置 IDF 环境
* 运行 `idf.py set-target esp32p4` 以设置目标芯片
* 如果上一步出现任何错误，请运行 `pip install "idf-component-manager~=1.1.4"` 来升级你的组件管理器
* 运行 `idf.py -p PORT flash monitor` 来构建、烧录并监控项目

（要退出串行监视器，请按 `Ctrl-]`。）

请参阅《入门指南》了解配置和使用 ESP-IDF 构建项目的所有步骤。

### PC 端

准备工作，请参考 [windows_driver](./windows_driver/README_cn.md)

![Demo](https://dl.espressif.com/AE/esp-iot-solution/p4_usb_extern_screen.gif)

## 其他问题

### 触摸屏控制的不是设备端的屏幕

* 在控制面板中选择 `平板电脑设置`

* 在配置栏中选择 `设置`

* 按照提示选择扩展屏

### 调高/调低 JPEG 的图片质量

* 修改 `CONFIG_USB_EXYEEND_SCREEN_JEPG_QUALITY`。该配置名沿用现有拼写以保持兼容。

### 修改副屏分辨率

* 保持 `CONFIG_USB_EXTEND_SCREEN_HEIGHT=1024` 和 `CONFIG_USB_EXTEND_SCREEN_WIDTH=576`，以使用最大横屏输入。
* Nano 面板本身为竖屏，固件会进行软件旋转并居中显示，多出的区域保持黑屏。

### 修改图像输出帧率

* 修改 `CONFIG_USB_EXTEND_SCREEN_MAX_FPS`，降低该值可以有效的减少 USB 带宽，当 USB AUDIO 音频卡顿时，可以适当减少此值。

### 修改一帧图像的最大值

* 修改 `CONFIG_USB_EXTEND_SCREEN_FRAME_LIMIT_B`，可以限制 PC 驱动传来的图像最大长度。

### 不启用触摸屏功能

* 修改 `CONFIG_HID_TOUCH_ENABLE` 为 `n`

### 不启用音频功能

* 修改 `CONFIG_UAC_AUDIO_ENABLE` 为 `n`

Note: 当只启用副屏功能，请将 PID 修改为 `0x2987`
