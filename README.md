
## 项目描述

一个基于 Qt 6 的跨平台串口调试助手，主要用于在开发过程中与下位机（如单片机、嵌入式设备）进行串口通信调试。程序提供图形界面，支持枚举本机串口、配置波特率、发送文本指令，并以日志形式实时显示接收到的数据，便于快速验证下位机的通信协议和数据输出。

<img width="789" height="646" alt="image" src="https://github.com/user-attachments/assets/6e53f3db-20ab-48e8-8d16-a2e62fd240ee" />

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
├── mainwindow.ui       # 界面布局：参数设置区、操作按钮区、日志显示区
├── image.png           # 软件界面截图
└── README.md           # 项目说明文档
```

整体为单窗口、事件驱动架构，各模块职责如下：

| 模块 | 职责 |
| --- | --- |
| `main.cpp` | 应用启动入口，负责初始化 Qt 应用与主窗口 |
| `MainWindow`（`mainwindow.h/.cpp`） | 核心控制器：管理 `QSerialPort` 生命周期、处理 UI 交互、解析接收数据并输出日志 |
| `mainwindow.ui` | 界面定义，包含串口选择、波特率设置、输入文本、功能按钮与日志浏览器 |
| `CMakeLists.txt` | 组织编译与链接，并生成部署脚本 |

## 核心实现

- **串口管理**：`QSerialPort` 以 `8N1`（8 数据位、无校验、1 停止位、无流控）、读写模式打开；仅在首次收发时按需打开，避免重复配置。
- **串口枚举**：通过 `QSerialPortInfo::availablePorts()` 刷新端口列表，刷新时保留当前选中项。
- **数据发送**：在输入框按回车，将文本以 UTF-8 编码写入串口并立即 `flush()`，同时记录发送日志。
- **数据接收**：`readyRead` 信号触发读取，数据先进入 `m_rxBuffer` 缓冲区：
  - 按 `\n` 拆分为行并去掉行尾 `\r`，逐行显示；
  - 对无换行结尾的残片，使用 50ms 单次 `QTimer` 延迟刷新后输出，避免粘包丢失；
  - 解码时优先按 UTF-8，若出现替换字符则回退到本地编码（`fromLocal8Bit`）。
- **日志展示**：`QTextBrowser` 输出带 `HH:mm:ss` 时间戳的日志，最大保留 2000 行，支持一键清空。
- **错误处理**：监听 `errorOccurred` 信号，将串口错误信息写入日志。

## 功能特性

- 刷新 / 选择串口，设置波特率（范围 1 ~ 10000000，默认 115200）
- 文本发送（回车触发）
- 实时接收与按行解析显示，自动兼容 UTF-8 与本地编码
- 带时间戳的收发日志，支持清空
- 串口异常提示

