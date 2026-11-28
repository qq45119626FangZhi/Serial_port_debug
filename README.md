
## 项目描述

一个基于 Qt 6 的跨平台串口调试助手，主要用于在开发过程中与下位机（如单片机、嵌入式设备）进行串口通信调试。程序提供图形界面，支持枚举本机串口、配置波特率、发送文本指令，并以日志形式实时显示接收到的数据，便于快速验证下位机的通信协议和数据输出。

<img width="1002" height="670" alt="9391ec185e43d50c9ea92b343f6df696" src="https://github.com/user-attachments/assets/73d6411e-816d-4f0b-a6cb-ccb1f12bb46c" />

可执行程序包：release/Serial_port_debug.exe 

## 技术栈

- **语言 / 标准**：C++
- **框架**：Qt 6.5+（Core、Widgets、SerialPort 模块）
- **构建系统**：CMake 3.19+，使用 `qt_standard_project_setup()` 与 `qt_add_executable()`
- **UI 描述**：Qt Designer 的 `.ui` 文件，编译时由 `uic` 自动生成 `ui_mainwindow.h`

## 项目架构

```
Serial_port_debug/
├── CMakeLists.txt      # 构建配置：查找 Qt6 组件、定义可执行文件、链接库与部署脚本
├── main.cpp            # 程序入口：创建 QApplication 与 MainWindow，进入事件循环
├── mainwindow.h        # 主窗口类声明：槽函数、串口对象、接收缓冲区与定时器
├── mainwindow.cpp      # 主窗口实现：串口初始化、参数配置、收发数据与日志显示
├── mainwindow.ui       # 界面布局：串口设置区、数据收发区、连接状态与日志显示区
├── image.png           # 软件界面截图
└── README.md           # 项目说明文档
```

整体为单窗口、事件驱动架构，各模块职责如下：

| 模块 | 职责 |
| --- | --- |
| `main.cpp` | 应用启动入口，负责初始化 Qt 应用与主窗口 |
| `MainWindow`（`mainwindow.h/.cpp`） | 核心控制器：管理 `QSerialPort` 生命周期、处理 UI 交互、解析接收数据并输出日志 |
| `mainwindow.ui` | 界面定义，包含串口设置（串口、波特率、刷新、关闭串口）、数据收发（输入框、HEX 发送、发送、清空日志）、连接状态标签与日志浏览器 |
| `CMakeLists.txt` | 组织编译与链接，并生成部署脚本 |

## 核心实现

- **串口管理**：`QSerialPort` 以 `8N1`（8 数据位、无校验、1 停止位、无流控）、读写模式打开；仅在首次收发时按需打开，避免重复配置。
- **串口枚举**：通过 `QSerialPortInfo::availablePorts()` 刷新端口列表，刷新时保留当前选中项。
- **数据发送**：点击「发送」按钮或在输入框按回车，将内容写入串口并立即 `flush()`；勾选「HEX发送」时按十六进制字节解析输入（如 `55 AA 01`），否则按 UTF-8 文本发送，并记录发送日志。
- **串口关闭**：点击「关闭串口」断开连接，清空接收缓冲区并更新连接状态。
- **连接状态**：界面标签实时显示「未连接」或「已连接: COMx @ 波特率」，波特率在串口打开时修改会即时生效。
- **数据接收**：`readyRead` 信号触发读取，数据先进入 `m_rxBuffer` 缓冲区：
  - 按 `\n` 拆分为行并去掉行尾 `\r`，逐行显示；
  - 对无换行结尾的残片，使用 50ms 单次 `QTimer` 延迟刷新后输出，避免粘包丢失；
  - 解码时优先按 UTF-8，若出现替换字符则回退到本地编码（`fromLocal8Bit`）。
- **日志展示**：`QTextBrowser` 输出带 `HH:mm:ss` 时间戳的日志，最大保留 2000 行，支持一键清空。
- **错误处理**：监听 `errorOccurred` 信号，将串口错误信息写入日志。

## 功能特性

- 刷新 / 选择串口，常用波特率下拉选择（默认 115200），支持关闭串口
- 文本 / HEX 发送（回车或按钮触发）
- 实时接收与按行解析显示，自动兼容 UTF-8 与本地编码
- 连接状态实时显示（未连接 / 已连接: COMx @ 波特率）
- 带时间戳的收发日志，支持清空
- 串口异常提示

## 构建与打包

直接双击项目根目录的 `build.bat`，脚本会一次性完成：

1. 初始化 MSVC 2022 x64 环境
2. CMake 配置 + Ninja 编译 Release 版本
3. 精简部署（仅保留必要的 Qt 运行库、平台插件和 VC 运行库 DLL）
4. 用 WinRAR 打包成单文件自解压程序 `Serial_port_debug.exe`（约 10 MB）

生成的可执行文件位于 `release/` 文件夹，可直接双击运行。构建中间产物在 `build/` 目录下。

> 构建脚本中的工具链路径（Qt 6.12.0、VS 2022、CMake、Ninja、WinRAR）需与本机环境一致，如路径不同请修改 `build.bat` 开头的变量。

