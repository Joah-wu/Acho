# Acho Android 客户端

Android Gradle工程，入口模块为`app/`。

主要功能：用户登录、SoftAP配网、二维码/ NFC绑定、设备媒体列表和文件上传管理。应用通过HTTP与Django服务通信，并使用Espressif配网协议把Wi-Fi凭据发送给ESP32。

## 交互边界

- 配网：扫描终端二维码后连接ESP32 SoftAP，不经Django传递路由器密码。
- 绑定：通过服务器签发的二维码或ST25DV上的临时token关联用户和设备。
- 媒体：应用上传文件和管理列表；终端再经ESP32主动下载到SD卡。

关键文件：

| 路径 | 作用 |
| --- | --- |
| `settings.gradle`、`build.gradle` | Gradle工程配置 |
| `app/` | Android应用代码与资源 |
| `sdk/` | 本地配网与视觉依赖 |
