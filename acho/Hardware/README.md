# 硬件资料

本目录保存当前实物硬件的设计和参考资料，不参与固件构建。

| 路径 | 内容 |
| --- | --- |
| `PCBs/Acho_Main` | 主板原理图、BOM、交互BOM和Gerber |
| `PCBs/Audio_Core` | 音频子板原理图、BOM、交互BOM和Gerber |
| `Datasheets` | STM32F446、ESP32-C3、WM8960和ST25DV04K手册 |
| `Solids` | STEP、SolidWorks及贴图文件 |
| `SHA256SUMS` | 当前资料文件的SHA-256完整性校验清单 |

以目录中实际保留的文件为准；删除或替换PCB资料后，应同步更新本说明和`SHA256SUMS`。

## 使用方式

- 设计/维修时先阅读对应板的原理图，再按BOM核对器件和封装。
- 制板使用对应Gerber压缩包；装配定位可打开交互BOM。
- 固件与电源、时钟、NFC或音频问题结合`Datasheets/`中的芯片手册排查。
- `SHA256SUMS`可用`sha256sum -c SHA256SUMS`核对当前保存的资料是否完整。
