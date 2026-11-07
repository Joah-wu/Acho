# Django 服务

项目入口为`manage.py`，Python配置包为`djangoProject/`，业务应用位于`appMyBroker/`。

| 路径 | 作用 |
| --- | --- |
| `djangoProject/settings.py` | 数据库、媒体、ASGI与Django配置 |
| `appMyBroker/models.py` | 用户、设备、绑定和媒体模型 |
| `appMyBroker/views.py` | Android HTTP接口 |
| `appMyBroker/file_transfer.py` | ESP32设备文件传输逻辑 |
| `requirements.txt` | Python依赖 |

使用迁移创建数据库后，通过ASGI/Django运行服务；媒体文件由服务端保存并按设备授权提供。

## 数据流

1. Android完成登录后创建或管理设备绑定、上传媒体。
2. ESP32请求绑定信息或目标文件，服务端校验设备归属。
3. 文件内容由ESP32转发给STM32，STM32校验后保存到SD卡。

数据库连接、密钥和媒体目录属于部署配置，应通过环境或本地设置维护，不写入客户端固件。
