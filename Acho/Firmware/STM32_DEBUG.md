# STM32 调试记录

## 1. WSL 映射调试器

在 Windows 管理员 PowerShell 中执行：

```powershell
usbipd list
usbipd detach --busid xx-xx
usbipd attach --wsl --busid xx-xx
```

WSL 中确认设备：

```bash
lsusb
```

`/dev/ttyACM0` 只用于串口日志，不是 OpenOCD 调试接口。

## 2. 安装 OpenOCD

```bash
sudo apt update
sudo apt install openocd
```

## 3. 只下载固件

只烧录、不启动 GDB 或 VS Code 调试：

```bash
pkill openocd || true
sudo /usr/bin/openocd -s /usr/share/openocd/scripts \
  -f ./DapLink.cfg \
  -c "program build/Release/Firmware.elf verify reset exit"
```

其中 `verify` 校验写入内容，`reset` 复位芯片，`exit` 完成后退出 OpenOCD。

## 4. OpenOCD 连接检查

`DapLink.cfg` 使用 CMSIS-DAP + SWD + STM32F4：

调试器如果显示为 `1a86:8012 QinHeng Electronics WCH-Link`，它是 WCH-Link，不是标准 DAPLink；需要确认其固件支持 CMSIS-DAP。

如果出现 `tpiu create ...`，说明 OpenOCD 二进制和脚本版本混用。固定使用同一套系统 OpenOCD：

```bash
/usr/bin/openocd -s /usr/share/openocd/scripts \
  -f ./DapLink.cfg -c "init; targets; shutdown"
```

## 5. USB 权限

如果只有 `sudo openocd` 能运行，添加 udev 规则：

```bash
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8012", MODE="0666", TAG+="uaccess"' \
  | sudo tee /etc/udev/rules.d/99-wch-link.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

重新执行 `usbipd attach` 后，确认普通用户也能运行：

```bash
openocd -f ./DapLink.cfg -c "init; targets; shutdown"
```

## 6. VS Code 调试

安装 `Cortex-Debug` 扩展，`.vscode/launch.json` 已配置两个入口：

- `Debug: Download and Debug`：使用 `build/Debug/Firmware.elf` 下载并进入 GDB 调试。
- `Release: Download Only`：使用 `build/Release/Firmware.elf` 只下载、校验、复位后退出，不启动 GDB。

在“运行和调试”面板顶部选择对应入口后点击开始按钮即可。

Debug 配置示例：

```json
{
    "type": "cortex-debug",
    "request": "launch",
    "cwd": "${workspaceFolder}",
    "executable": "${workspaceFolder}/build/Debug/Firmware.elf",
    "servertype": "openocd",
    "configFiles": ["${workspaceFolder}/DapLink.cfg"],
    "serverpath": "/usr/bin/openocd",
    "searchDir": ["/usr/share/openocd/scripts"],
    "gdbPath": "/home/joahwu/.local/share/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin/arm-none-eabi-gdb",
    "runToEntryPoint": "main"
}
```

修改配置后重载 VS Code 窗口，再启动调试。
启动调试前如有残留 OpenOCD 进程，先执行：

```bash
pkill openocd || true
```

只下载、不调试使用 `.vscode/tasks.json` 中的 `STM32: Download Firmware` 任务：

`Ctrl+Shift+P` → `Tasks: Run Task` → `STM32: Download Firmware`

## 7. 串口日志

```bash
screen /dev/ttyACM0 115200
# 或
picocom -b 115200 /dev/ttyACM0
```

