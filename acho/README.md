# Acho 终端工程

本目录包含终端硬件资料与两颗MCU的固件。

| 目录 | 内容 |
| --- | --- |
| `Firmware/stm32f446` | STM32F446终端固件 |
| `Firmware/esp32c3` | ESP32-C3网络固件 |
| `Firmware/common` | 两端共用协议与NDEF代码 |
| `Hardware` | PCB、BOM、Gerber、手册和结构文件 |

## 功能边界

- STM32处理显示、按键、SD卡、音频、NFC和本地文件安装。
- ESP32处理SoftAP配网、NVS、服务器访问和UART分块转发。
- `common`保证两端对帧、CRC、Wi-Fi状态和NDEF内容使用同一格式。

硬件连接要点：PC2控制ESP32 EN；STM32 USART2 PA2/PA3连接ESP32 GPIO20/GPIO21；SD卡使用SDIO；显示、WM8960与ST25DV由STM32本地管理。详细引脚、模块和构建入口见各子目录README。
