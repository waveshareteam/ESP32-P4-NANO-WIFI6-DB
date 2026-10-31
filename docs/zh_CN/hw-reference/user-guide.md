# ESP32-P4-NANO-WIFI6-DB <!-- omit in toc -->

[English](../../en/hw-reference/user-guide.md)

本指南将帮助你快速上手 ESP32-P4-NANO-WIFI6-DB，并提供更深入的硬件信息。

ESP32-P4-NANO-WIFI6-DB 是一款以 ESP32-P4NRW32X 为主处理器、通过 SDIO 搭配 ESP32-C5-MINI-1U 无线协处理器的开发板。板载双 lane MIPI-DSI 显示与触摸接口、双 lane MIPI-CSI 摄像头接口、ES8311 音频、microSD、RMII 以太网、USB 2.0 Host 和两组 2×13 扩展排针。

本文档包括以下主要内容：

- [入门指南](#1-入门指南)：开发板概述、板载资源、外设和出厂固件。
- [开始开发应用](#2-开始开发应用)：所需硬件、供电方式和上手步骤。
- [硬件参考](#3-硬件参考)：GPIO 分配、扩展排针、接口、晶振电路和使用注意事项。
- [相关文档](#4-相关文档)：相关文档链接。

> **说明**
>
> 本文根据[官方文档](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/)整理，电路连接细节见[原理图](../../../hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf)。

# 1. 入门指南

## 1.1. 概述

ESP32-P4NRW32X 主处理器负责显示、摄像头、音频、存储、以太网和 USB 接口。ESP32-C5-MINI-1U 作为无线协处理器，通过 ESP-Hosted（4-bit SDIO）为 ESP32-P4 提供 2.4 / 5 GHz Wi-Fi 6 和 Bluetooth 5 LE。ESP32-P4 本身不集成 Wi-Fi 和蓝牙，因此无线功能需要 P4 应用与 C5 协处理器固件配合运行。

## 1.2. 板载资源

下表列出开发板的主要组件。

<details open>
<summary><b>板载资源表</b></summary>

| 主要组件 | 说明 |
| --- | --- |
| 主处理器 | ESP32-P4NRW32X，双核高性能 RISC-V 系统 + 单核低功耗系统，封装内集成 16 MB PSRAM。 |
| Flash | 16 MB 外置闪存。 |
| 无线协处理器 | ESP32-C5-MINI-1U，外接天线，支持 2.4 / 5 GHz Wi-Fi 6 和 Bluetooth 5 LE，通过 4-bit SDIO 与 ESP32-P4 连接。ESP32-P4 GPIO54 为 C5 使能/复位（高电平使能，低电平复位），GPIO6 接 C5 IO2。 |
| 以太网 | RMII MAC + IP101GRI PHY，PHY 地址为 1，50 MHz REF_CLK 外部输入，百兆 RJ45 接口，并提供兼容的选配 PoE 模块接口。 |
| 显示 | 双 lane MIPI-DSI，外接微雪屏幕（见[显示与摄像头接口](#33-显示与摄像头接口)）。LCD 复位引脚未接，背光通过 I2C 控制。 |
| 触摸 | GT911 电容触摸，接在 GPIO7/GPIO8 共享 I2C 总线上；I2C 地址为 0x5D 或 0x14，复位和中断未接到 ESP32-P4，触摸状态需通过 I2C 轮询读取。 |
| 音频 | ES8311 编解码器，I2C + I2S，MCLK 接 GPIO13；扬声器经功放输出，功放由 GPIO53 使能；单路板载模拟麦克风输入。 |
| 存储 | microSD 卡槽，4-bit SDMMC（槽位 0）；卡电源由 GPIO45 控制（低电平有效），IO 电压由片内 LDO 通道 4 提供。 |
| 摄像头 | 双 lane MIPI-CSI 接口，SCCB 接共享 I2C 总线，支持 OV5647 摄像头模组。摄像头接口上的 CSI_IO0/CSI_IO1 只做上拉，复位和掉电信号未接到 ESP32-P4。 |
| USB | USB 2.0 高速 OTG，Type-A 接口，接 ESP32-P4 内置 USB PHY。 |
| UART0 | 固件下载和串口日志，GPIO37（TX）/ GPIO38（RX），经板载 USB Type-C 接口引出，该接口同时提供 5 V 供电。 |
| 扩展排针 | 两组 2×13 排针（见[扩展排针](#32-扩展排针)），引出电源、I2C、UART 和 GPIO，部分 GPIO 与板载功能共用。 |
| 按键 / 指示灯 | BOOT、RESET 按键和电源指示灯。 |
| 32.768 kHz 晶振 | 接 GPIO0/GPIO1（XTAL_32K_N/P），为 ESP32-P4 提供 RTC 慢时钟。 |

</details>

## 1.3. 外设速查

<details open>
<summary><b>外设速查表</b></summary>

| 模块 | 器件 / 功能 | 接口 | 地址 / 参数 | GPIO / 信号 |
| --- | --- | --- | --- | --- |
| LCD | MIPI-DSI 显示屏 | MIPI-DSI | 2-lane；HX8394 / ILI9881C / JD9365；RGB565 或 RGB888 | LCD 复位未接 |
| LCD 背光 | 背光控制器 | I2C | 7-bit 地址 0x45；亮度寄存器 0x96，数据 0–255 | SDA=GPIO7，SCL=GPIO8 |
| 触摸 | GT911 电容触摸 | I2C | 7-bit 地址 0x5D 或 0x14 | SDA=GPIO7，SCL=GPIO8；RST/INT 未接 |
| 摄像头 | MIPI-CSI 摄像头接口 | MIPI-CSI + SCCB | 2-lane；OV5647 | SCCB 复用 GPIO7/8；复位和掉电信号未接 |
| 音频 | ES8311 编解码器 | I2C + I2S | 7-bit 地址 0x18 | SDA=7，SCL=8；MCLK=13，SCLK=12，WS=10，DOUT=9，DIN=11 |
| 功放 | 扬声器功放使能 | GPIO | 高电平有效 | GPIO53 |
| microSD | microSD 卡槽 | SDMMC 4-bit | IO 电压由片内 LDO 通道 4 提供 | CLK=43，CMD=44，D0–D3=39–42；电源使能=45，低电平有效 |
| 以太网 | IP101GRI PHY | RMII + SMI | PHY 地址 1；50 MHz REF_CLK 输入 | CRS_DV=28，RXD0=29，RXD1=30，TXD0=34，TXD1=35，TX_EN=49，REF_CLK=50；MDC=31，MDIO=52；PHY 复位=51 |
| Wi-Fi/BT 协处理器 | ESP32-C5-MINI-1U | SDIO 4-bit | — | CLK=18，CMD=19，D0–D3=14–17；使能/复位=54；C5 IO2=6 |
| USB | USB 2.0 高速 OTG | USB | 内置 USB PHY | 专用 DM/DP 引脚，Type-A 接口 |
| UART0 | 烧录与串口日志 | UART | — | TX=GPIO37，RX=GPIO38 |
| 按键 | BOOT / RESET | GPIO | BOOT 在上电时选择下载模式 | BOOT=GPIO35；RESET 复位开发板 |

</details>

## 1.4. 出厂固件

开发板的应用固件基于 ESP-Brookesia，包含显示、触摸、音频、摄像头、视频、Wi-Fi、以太网和小智等功能。功能介绍、使用方法和编译说明见 [Brookesia 固件 README](../../../firmware/brookesia/README_CN.md)。ESP32-P4 主控和 ESP32-C5 协处理器的预编译固件见 [`firmware/bin/`](../../../firmware/bin/)，两者目标芯片和烧录地址不同，烧录前请核对对应说明和固件版本。

# 2. 开始开发应用

本节介绍使用开发板所需的硬件、供电方式和上手步骤。

## 2.1. 必备硬件

- 1 x ESP32-P4-NANO-WIFI6-DB
- 1 x USB Type-C 线，用于供电、固件下载和串口日志
- 1 x 电脑（Windows、Linux 或 macOS）

## 2.2. 可选硬件

- 1 x 微雪 MIPI-DSI 触摸屏：[5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm)、[7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm)、[8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) 或 [10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm)
- 1 x MIPI-CSI 摄像头模组（OV5647，如 [RPi Camera (B)](https://www.waveshare.net/shop/RPi-Camera-B.htm)）
- 1 x microSD 卡
- 1 x 扬声器
- 1 x 网线；PoE 供电时还需要选配 PoE 模块以及支持 IEEE 802.3af 的 PoE 路由器或交换机

## 2.3. 供电方式

开发板可通过以下任一方式供电：

- USB Type-C 接口，5 V 输入
- 以太网口 PoE 供电，需安装选配 PoE 模块，并连接支持 IEEE 802.3af 的路由器或交换机

> **注意**
>
> 固件无法检测开发板是否由 PoE 供电。验证 PoE 时，请在仅用 PoE 供电的情况下重新测试一遍。

## 2.4. 硬件准备

1. 用 USB Type-C 线将开发板的下载/串口接口连接到电脑。
2. 如使用屏幕或摄像头，请在断电状态下连接排线，并对照模组说明确认接口方向。
3. 如使用以太网，请连接到有 DHCP 服务器的网络；PoE 供电时请使用 IEEE 802.3af 端口。
4. 如自动下载失败，请按住 BOOT 键再复位，手动让开发板进入下载模式。

## 2.5. 上手建议

1. 先用板载 USB Type-C 接口连接电脑，确认电源指示灯点亮、串口能看到启动日志。
2. 再逐项验证实际要用的接口：屏幕与触摸、摄像头、microSD、音频、以太网和 Wi-Fi。
3. 开发环境搭建、示例工程和编译烧录步骤见仓库 [README](../../../README_CN.md)，出厂固件的使用方法见 [Brookesia 固件说明](../../../firmware/brookesia/README_CN.md)。

# 3. 硬件参考

本节提供开发板硬件的详细信息。

## 3.1. GPIO 分配

下表列出全部 GPIO（0–54）及其板载连接。

<details open>
<summary><b>GPIO 分配表（GPIO0–GPIO54）</b></summary>

| GPIO | 信号 | 连接到 | 备注 |
| --- | --- | --- | --- |
| GPIO0 | XTAL_32K_N | 32.768 kHz 晶振 / 扩展排针 | 作普通 GPIO 使用建议去掉 R32（见 [32.768 kHz 晶振与 GPIO0/GPIO1](#34-32768-khz-晶振与-gpio0gpio1r32r36)） |
| GPIO1 | XTAL_32K_P | 32.768 kHz 晶振 / 扩展排针 | 作普通 GPIO 使用建议去掉 R36（见 [32.768 kHz 晶振与 GPIO0/GPIO1](#34-32768-khz-晶振与-gpio0gpio1r32r36)） |
| GPIO2 | GPIO2 | 扩展排针 | 可配置为普通 GPIO |
| GPIO3 | GPIO3 | 扩展排针 | 可配置为普通 GPIO |
| GPIO4 | GPIO4 | 扩展排针 | 可配置为普通 GPIO |
| GPIO5 | GPIO5 | 扩展排针 | 可配置为普通 GPIO |
| GPIO6 | C5_IO2 | ESP32-C5 IO2 / 扩展排针 | 与 ESP32-C5 共用 |
| GPIO7 | I2C SDA | ES8311、GT911、LCD 背光控制器、摄像头 SCCB / 扩展排针 | 共享 I2C 总线 |
| GPIO8 | I2C SCL | ES8311、GT911、LCD 背光控制器、摄像头 SCCB / 扩展排针 | 共享 I2C 总线 |
| GPIO9 | I2S DOUT | ES8311 DSDIN | 播放数据，ESP32-P4 → codec |
| GPIO10 | I2S WS | ES8311 LRCK | 音频帧同步 |
| GPIO11 | I2S DIN | ES8311 ASDOUT | 采集数据，codec → ESP32-P4 |
| GPIO12 | I2S SCLK | ES8311 SCLK | 音频位时钟 |
| GPIO13 | I2S MCLK | ES8311 MCLK | 音频主时钟 |
| GPIO14 | SDIO D0 | ESP32-C5 | ESP-Hosted SDIO 数据线 |
| GPIO15 | SDIO D1 | ESP32-C5 | ESP-Hosted SDIO 数据线 |
| GPIO16 | SDIO D2 | ESP32-C5 | ESP-Hosted SDIO 数据线 |
| GPIO17 | SDIO D3 | ESP32-C5 | ESP-Hosted SDIO 数据线 |
| GPIO18 | SDIO CLK | ESP32-C5 | ESP-Hosted SDIO 时钟 |
| GPIO19 | SDIO CMD | ESP32-C5 | ESP-Hosted SDIO 命令 |
| GPIO20 | GPIO20 | 扩展排针 | 可配置为普通 GPIO |
| GPIO21 | GPIO21 | 扩展排针 | 可配置为普通 GPIO |
| GPIO22 | GPIO22 | 扩展排针 | 可配置为普通 GPIO |
| GPIO23 | GPIO23 | 扩展排针 | 可配置为普通 GPIO |
| GPIO24 | USB1P1_N0 | 扩展排针 | 与 USB 功能复用 |
| GPIO25 | USB1P1_P0 | 扩展排针 | 与 USB 功能复用 |
| GPIO26 | USB1P1_N1 | 扩展排针 | 与 USB 功能复用 |
| GPIO27 | USB1P1_P1 | 扩展排针 | 与 USB 功能复用 |
| GPIO28 | RMII CRS_DV | 以太网 PHY | 载波侦听 / 接收数据有效，PHY → ESP32-P4 |
| GPIO29 | RMII RXD0 | 以太网 PHY | 接收数据位 0，PHY → ESP32-P4 |
| GPIO30 | RMII RXD1 | 以太网 PHY | 接收数据位 1，PHY → ESP32-P4 |
| GPIO31 | SMI MDC | 以太网 PHY | PHY 管理时钟，ESP32-P4 → PHY |
| GPIO32 | GPIO32 | 扩展排针 | 可配置为普通 GPIO |
| GPIO33 | GPIO33 | 扩展排针 | 可配置为普通 GPIO |
| GPIO34 | RMII TXD0 | 以太网 PHY | 发送数据位 0，ESP32-P4 → PHY；Strapping 引脚 |
| GPIO35 | RMII TXD1 / BOOT | 以太网 PHY、BOOT 按键 | 发送数据位 1，ESP32-P4 → PHY；Strapping 引脚，上电时选择下载模式 |
| GPIO36 | GPIO36 | 扩展排针 | 可配置为普通 GPIO；Strapping 引脚，外部上拉 |
| GPIO37 | UART0 TX | 下载/串口 / 扩展排针 | 固件下载和串口日志；Strapping 引脚 |
| GPIO38 | UART0 RX | 下载/串口 / 扩展排针 | 固件下载和串口日志；Strapping 引脚 |
| GPIO39 | SD D0 | microSD 卡 | SDMMC 数据线 |
| GPIO40 | SD D1 | microSD 卡 | SDMMC 数据线 |
| GPIO41 | SD D2 | microSD 卡 | SDMMC 数据线 |
| GPIO42 | SD D3 | microSD 卡 | SDMMC 数据线 |
| GPIO43 | SD CLK | microSD 卡 | SDMMC 时钟 |
| GPIO44 | SD CMD | microSD 卡 | SDMMC 命令 |
| GPIO45 | SD_VDD_EN | microSD 电源开关 / 扩展排针 | 低电平有效；SD 卡工作期间请勿作为 GPIO 使用 |
| GPIO46 | GPIO46 | 扩展排针 | 可配置为普通 GPIO |
| GPIO47 | GPIO47 | 扩展排针 | 可配置为普通 GPIO |
| GPIO48 | GPIO48 | 扩展排针 | 可配置为普通 GPIO |
| GPIO49 | RMII TX_EN | 以太网 PHY | 发送使能，ESP32-P4 → PHY |
| GPIO50 | RMII REF_CLK | 以太网 PHY | 50 MHz 参考时钟，PHY → ESP32-P4 |
| GPIO51 | PHY_RST | 以太网 PHY | PHY 硬件复位，ESP32-P4 → PHY |
| GPIO52 | SMI MDIO | 以太网 PHY | PHY 管理数据，双向 |
| GPIO53 | PA_CTRL | 扬声器功放 / 扩展排针 | 高电平有效 |
| GPIO54 | C5_CHIP_PU | ESP32-C5 EN / 扩展排针 | 低电平复位，高电平使能 |

</details>

## 3.2. 扩展排针

开发板有两组 2×13（2.54 mm）扩展排针 P1 和 P2，引出电源、I2C、UART 和 GPIO。下表的左右位置以开发板的实际方向为准：USB Type-C 接口朝上，RJ45 网口朝下，左侧为 P1、右侧为 P2，每组排针顶部为 1、2 脚，底部为 25、26 脚。带丝印的引脚图见[官方文档](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/)。标 \* 的信号与板载功能共用，外接电路前请参阅[使用注意事项](#35-使用注意事项)。

<details open>
<summary><b>扩展排针（2 × 2×13）</b></summary>

<center>

| ![扩展排针 P1、P2 原理图](../../../assert/Header.webp) |
|:--:|
| 扩展排针 P1（左）和 P2（右）原理图 |

</center>

<table>
  <thead>
    <tr>
      <th colspan="2">左侧排针 P1</th>
      <th colspan="2">右侧排针 P2</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>3V3</td><td>5V</td><td>5V</td><td>ESP_LDO_VO4</td></tr>
    <tr><td>GPIO7（I2C SDA）*</td><td>5V</td><td>GND</td><td>GND</td></tr>
    <tr><td>GPIO8（I2C SCL）*</td><td>GND</td><td>3V3</td><td>GPIO0（XTAL_32K_N）*</td></tr>
    <tr><td>GPIO23</td><td>GPIO37（UART0 TX）*</td><td>GND</td><td>GPIO1（XTAL_32K_P）*</td></tr>
    <tr><td>GND</td><td>GPIO38（UART0 RX）*</td><td>GPIO3</td><td>GND</td></tr>
    <tr><td>GPIO5</td><td>GPIO4</td><td>GPIO2</td><td>GPIO6（C5 IO2）*</td></tr>
    <tr><td>GPIO20</td><td>GND</td><td>GPIO54（C5 使能）*</td><td>GPIO53（功放使能）*</td></tr>
    <tr><td>GPIO21</td><td>GPIO22</td><td>GPIO47</td><td>GPIO48</td></tr>
    <tr><td>3V3</td><td>GPIO24（USB1P1_N0）</td><td>GPIO46</td><td>GND</td></tr>
    <tr><td>GPIO25（USB1P1_P0）</td><td>GND</td><td>GPIO45（SD 电源）*</td><td>C5_U0RXD</td></tr>
    <tr><td>GPIO26（USB1P1_N1）</td><td>GPIO27（USB1P1_P1）</td><td>USBD_P</td><td>C5_U0TXD</td></tr>
    <tr><td>GPIO32</td><td>GPIO33</td><td>USBD_N</td><td>C5_BOOT</td></tr>
    <tr><td>GND</td><td>GPIO36</td><td>GND</td><td>GND</td></tr>
  </tbody>
</table>

</details>

- 空闲 GPIO：GPIO2–5、GPIO20–27、GPIO32、GPIO33、GPIO36、GPIO46–48。
- 表中 GPIO 编号均属于 ESP32-P4；`C5_U0RXD`、`C5_U0TXD` 和 `C5_BOOT` 是 ESP32-C5 的下载信号，不属于 P4 的 UART0。
- `USBD_P`、`USBD_N` 是 USB 数据差分信号；GPIO24–GPIO27 另标有 `USB1P1_N0/P0/N1/P1` 复用功能，连接时应区分。
- `ESP_LDO_VO4` 是电源信号，不是 GPIO，也不是相邻的 5V 引脚。
- GPIO7/GPIO8 是板载 I2C 总线，可在此外接 I2C 设备，但需避免地址冲突。
- GPIO37/GPIO38 是用于烧录和日志的 UART0。
- 官方引脚图中的 ADC 和触摸通道标注表示对应 GPIO 的芯片复用功能，不会自动解除板载连接或占用。

## 3.3. 显示与摄像头接口

摄像头 SCCB、GT911 触摸控制器、LCD 背光控制器和 ES8311 共用 GPIO7/GPIO8 上的 I2C。

| 接口 | 类型 | ESP32-P4 信号 |
| --- | --- | --- |
| 显示 | MIPI-DSI | 两条数据 lane、时钟 lane，触摸和背光使用共享 I2C；LCD 复位、触摸复位和触摸中断未接 |
| 摄像头 | MIPI-CSI | 两条数据 lane、时钟 lane，SCCB 使用共享 I2C；复位和掉电信号未接 |

可外接的微雪屏幕如下。各屏幕的控制器和时序不同，固件需要与实际连接的屏幕匹配。

| 屏幕 | 控制器 | 分辨率 | DSI lane 速率 |
| --- | --- | --- | --- |
| [5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm) | HX8394，5 英寸 | 720 × 1280 | 700 Mbps |
| [7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm) | ILI9881C，7 英寸 | 720 × 1280 | 1000 Mbps |
| [8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) | JD9365，8 英寸 | 800 × 1280 | 1500 Mbps |
| [10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) | JD9365，10.1 英寸 | 800 × 1280 | 1500 Mbps |

## 3.4. 32.768 kHz 晶振与 GPIO0/GPIO1（R32/R36）

GPIO0 和 GPIO1 既引到扩展排针（P2 的 6、8 脚），也接在 32.768 kHz 晶振电路上：GPIO0 经 R32（0 Ω）接晶振 Y2 的 2 脚，GPIO1 经 R36（0 Ω）接 Y2 的 1 脚，R34（10 MΩ）是跨接在两脚之间的反馈电阻，C16/C21（22 pF）是负载电容。因此默认状态下把 GPIO0/GPIO1 当普通 GPIO 驱动，会同时驱动晶振回路。

<center>

| ![32.768 kHz 晶振电路](../../../assert/XTAL.webp) |
|:--:|
| 32.768 kHz 晶振电路 |

</center>

| 配置 | R32 / R36 | 32.768 kHz 晶振 | GPIO0 / GPIO1 |
| --- | --- | --- | --- |
| 默认 | 0 Ω | 接到 GPIO0/GPIO1，提供 RTC 慢时钟 | 不建议单独作为普通 GPIO 使用 |
| GPIO0/GPIO1 作 GPIO | 去掉 | 与 ESP32-P4 断开 | 可在 P2 的 6、8 脚自由使用 |

- 如果要单独使用 GPIO0 和 GPIO1 作为通用 GPIO，建议拆掉 R32 和 R36，把晶振从这两个引脚上断开。
- 拆掉后 ESP32-P4 失去外部 32.768 kHz 慢时钟，RTC 慢时钟需要在工程中改用内部 RC 时钟源。
- 只需要其中一个引脚时，也要两颗电阻一起拆：晶振两端仍接在一起的话，另一端会跟着被驱动。

## 3.5. 使用注意事项

- GPIO7/8 I2C 总线上已有 ES8311（0x18）、GT911（0x5D/0x14）、LCD 背光控制器（0x45）和摄像头 SCCB，外接 I2C 设备时请避免地址冲突。
- GPIO6、GPIO45、GPIO53、GPIO54 是板载外设的控制信号（ESP32-C5 IO2、microSD 电源、扬声器功放、ESP32-C5 使能），对应外设工作期间请勿单独驱动。
- GPIO28–GPIO31、GPIO34–GPIO35、GPIO49–GPIO52 用于以太网 RMII/SMI 信号和 PHY 复位，请勿作为普通 GPIO 使用。
- GPIO34–GPIO38 是 ESP32-P4 的 Strapping 引脚，在上电和复位时被采样。GPIO34/35 同时用于以太网 TXD0/TXD1，GPIO35 还参与 BOOT 模式选择，GPIO36 引出到扩展排针，GPIO37/38 用于烧录和日志；请确保外接电路不会在采样期间干扰其电平。
- GPIO0/GPIO1 接 32.768 kHz 晶振，要单独作为普通 GPIO 使用时建议拆掉 R32 和 R36，见 [32.768 kHz 晶振与 GPIO0/GPIO1（R32/R36）](#34-32768-khz-晶振与-gpio0gpio1r32r36)。
- GPIO24–GPIO27 带有 USB 复用功能，作普通 GPIO 使用前请确认不会与 USB 功能冲突。
- LCD 复位、GT911 复位和 GT911 中断均未接线，触摸状态通过 I2C 轮询读取。
- ESP32-P4 和 ESP32-C5 的固件相互独立，更新其中一个不会同步更新另一个。
- 请在断电状态下连接屏幕/摄像头排线，并对照模组说明确认接口方向。

# 4. 相关文档

## 4.1. 数据手册与原理图

- [ESP32-P4-NANO-WIFI6-DB 原理图](../../../hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf)（PDF）
- [ESP32-P4 技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_cn.pdf)（PDF）
- [ESP32-C5 技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-c5_datasheet_cn.pdf)（PDF）

板卡外形与安装孔位见[官方文档](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/)。

## 4.2. 软件资源

- [Brookesia 固件](../../../firmware/brookesia/README_CN.md)
- [预编译固件](../../../firmware/bin/)
- [ESP-IDF 示例](../../../examples/esp_idf/)和 [Arduino 示例](../../../examples/Arduino/)
- [官方文档](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/)和[产品页](https://www.waveshare.net/shop/ESP32-P4-NANO-WIFI6-DB.htm)
