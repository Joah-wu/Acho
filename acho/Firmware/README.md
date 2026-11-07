# Acho 固件

| 目录 | 目标与主要实现 |
| --- | --- |
| `stm32f446` | 终端UI、SD卡、音频、NFC、串口接收与外设控制 |
| `esp32c3` | Wi-Fi配网、网络请求、分块文件桥接 |
| `common` | CRC帧、状态结构和NDEF编码 |

STM32与ESP32通过115200 8N1串口交换带CRC和ACK的帧。STM32 USART2负责协议，日志通过SWD/RTT读取；ESP32 UART0在正常运行时不输出控制台日志。

## 数据流

1. STM32发出状态、二维码或文件请求。
2. ESP32本地回复配网信息，或向Django请求绑定数据和媒体分块。
3. ESP32以BEGIN、DATA、END帧发送内容；STM32校验并ACK。
4. 文件校验通过后安装到SD卡，LVGL和音频模块按需读取。

## 关键配置

- STM32：`stm32f446/Firmware.ioc`、`CMakePresets.json`。
- ESP32：`esp32c3/sdkconfig`、`sdkconfig.defaults`、`main/idf_component.yml`。
- 共享协议：`common/acho_protocol.h`。

STM32工程只在`stm32f446/`目录构建；ESP32工程在`esp32c3/`目录使用ESP-IDF构建。`build/`为可再生输出，不纳入版本库。
