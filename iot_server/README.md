# Acho 服务端

服务端工作区，实际Django项目位于`djangoProject/`。

服务负责账号、设备、绑定关系、媒体文件和设备下载接口；Android客户端通过HTTP访问，ESP32通过网络桥获取状态、绑定数据和媒体分块。

## 模块关系

- Android调用用户登录、设备绑定和媒体管理接口。
- ESP32以设备身份请求配网后服务、绑定数据和文件内容。
- Django根据用户、设备及文件归属决定可访问的数据，并保存媒体文件。

依赖、启动方式、模型和接口说明见[djangoProject README](djangoProject/README.md)。
