# 共享协议

STM32F446与ESP32-C3共同编译本目录的代码，保证两端使用相同的数据定义。

| 文件 | 作用 |
| --- | --- |
| `acho_protocol.h/.c` | 串口帧头、类型、长度、CRC32与编解码 |
| `acho_wifi_info.h` | Wi-Fi热点、PoP和状态数据结构 |
| `acho_ndef.h/.c` | ST25DV使用的NDEF/TLV编码 |

协议帧用于状态查询、二维码、绑定token和文件分块传输；文件传输通过BEGIN、DATA、END和ACK完成完整性校验。

## 协议职责

| 类型 | 作用 |
| --- | --- |
| REQUEST | STM32请求Wi-Fi信息、二维码、绑定或文件 |
| BEGIN | 声明一次响应或文件传输的总长度/元数据 |
| DATA | 承载分块数据 |
| END | 提交累计长度与文件CRC |
| ACK / ERROR | 确认已接收或报告失败 |

两端修改协议时必须同时更新本目录，不应各自在STM32或ESP32目录复制结构定义。
