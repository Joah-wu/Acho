# image_encoder

此目录仅保存上游 [image_dither_tool](https://github.com/Joah-wu/image_dither_tool) 的`release`发布包及其校验文件。

`lvgl_dither`用于把图片转换为STM32 SD卡读取的LVGL BIN素材。工具本身不参与STM32、ESP32、Android或Django构建；解压目录和临时转换输出均应保持本地忽略。

转换后的BIN按用途复制到SD卡的`Static/`或媒体目录。动画帧需要保持既有目录、编号、画布尺寸和像素格式；工具发布包的版本与校验文件位于`release/`。
