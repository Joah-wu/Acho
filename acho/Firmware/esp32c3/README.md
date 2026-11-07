# ESP32-C3 固件

ESP-IDF 5.2工程。入口为`main/esp32c3.c`，构建配置为`CMakeLists.txt`、`sdkconfig`与`main/idf_component.yml`。

| 文件 | 实现 |
| --- | --- |
| `main/acho_wifi.c` | SoftAP配网、NVS保存、设备注册和重配网 |
| `main/acho_bridge.c` | UART帧与HTTP/WebSocket请求之间的分块桥接 |
| `main/acho_wifi_qr.c` | 配网二维码生成 |
| `main/esp32c3.c` | 启动Wi-Fi与桥接任务 |

UART0使用GPIO21 TX、GPIO20 RX，115200 8N1连接STM32 USART2。正常配置为`Console None`，避免日志干扰协议；下载时由STM32的ESP32 Download页面释放共享线路。

## 工作流程

1. 首次启动创建SoftAP，生成设备名、PoP和配网二维码。
2. Android通过Security 1会话发送Wi-Fi凭据与服务器地址。
3. 联网后，ESP32保存配置、注册设备并为STM32处理状态/二维码请求。
4. 文件下载时ESP32从服务端逐块取数，等待STM32 ACK后继续下一块，避免整文件占用RAM。

## 配置与边界

- `sdkconfig`保存当前编译配置，`sdkconfig.defaults`提供新配置的默认值。
- 服务器地址等运行数据保存在NVS，不写入STM32固件。
- UART0只承载桥接协议；需要诊断时使用STM32的SWD/RTT，不在正常版本混入控制台日志。

## 构建

```bash
. /home/joahwu/esp32/esp-idf/export.sh
idf.py build
```

烧录参数和Flash频率以当前`build/flash_args`与`sdkconfig`为准。
