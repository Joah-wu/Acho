# STM32F446 固件

STM32F446RET6终端固件，CubeMX配置位于`Firmware.ioc`，CMake入口为`CMakeLists.txt`。

## 主要目录

| 路径 | 实现 |
| --- | --- |
| `Core/` | CubeMX生成的启动、外设与FreeRTOS基础代码 |
| `User/gui/` | LVGL页面：主菜单、播放、配网、NFC、绑定、下载模式、闪光灯 |
| `User/transfer_link.c` | ESP32 UART协议、SD/RAM接收与回调 |
| `User/WM8960Apps.c` | WAV解析、I²S DMA播放与音量 |
| `User/nfc_service.c` | ST25DV NDEF发布、RF忙处理和token生命周期 |
| `FATFS/`、`Lib/Lvgl/`、`Lib/wm8960/` | 文件系统、图形与音频驱动 |

已实现SDIO素材读取、LVGL动画、16-bit PCM播放、NFC配网/绑定提示、ESP32复位/下载模式、闪光灯和蜂鸣器。日志使用SWD/RTT；USART1保持高阻，USART2与ESP32通信。

## 运行架构

| 上下文 | 职责 |
| --- | --- |
| GUI任务 | LVGL页面、编码器事件、SD素材与传输回调 |
| audio任务 | WAV解析、WM8960配置、I²S DMA半缓冲补数 |
| transfer任务 | UART帧解析、ACK、SD/RAM写入和超时处理 |
| NFC服务 | ST25DV读写、NDEF发布、RF忙和token过期处理 |
| DMA/定时器中断 | 显示/音频完成通知、蜂鸣器时序等短时事件 |

## 资源约定

- SD卡的`Static/`保存动画、NFC提示图和界面素材，`Audios/`保存WAV。
- LVGL文件系统使用`P:`映射SD卡；素材缺失时页面提供文字回退。
- WAV播放支持16-bit PCM、单/双声道以及16/44.1/48 kHz采样率。
- `PC2/WIFI_EN`用于ESP32硬复位；ESP32下载模式会释放共享UART线路。

## 构建

在本目录执行：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Release使用对应的`Release`预设。不要在上级`Firmware/`目录创建STM32构建目录。
