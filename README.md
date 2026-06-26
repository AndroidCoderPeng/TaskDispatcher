# TaskDispatcher - 任务调度器

一个基于 Qt 开发的现代化任务调度管理工具，支持 WebSocket 通信、多通知渠道和灵活的定时任务管理。

## ✨ 功能特性

- 📅 **任务管理**：每日任务自动刷新与重置
- 🔔 **多渠道通知**：支持邮箱和企业微信通知
- 🌐 **WebSocket 服务**：内置 WebSocket 服务端，支持实时通信
-  **双主题切换**：提供亮色和深色两种 macOS 风格主题
- ⚙️ **灵活配置**：支持任务超时、随机时间、节假日跳过等高级设置
- 💾 **数据同步**：支持节假日数据同步和本地数据导入导出

## ️ 技术栈

- **框架**: Qt 5 (Widgets)
- **语言**: C++11
- **网络**: Qt Network, Qt WebSockets
- **数据库**: Qt SQL
- **构建工具**: qmake

## 📦 项目结构

```
TaskDispatcher/
├── main.cpp                    # 程序入口
├── DispatcherApplication.hpp   # 应用程序类
├── DispatcherApplication.cpp
├── MainWindow.hpp              # 主窗口类
├── MainWindow.cpp
├── MainWindow.ui               # UI 界面设计
├── WebSocketObserver.hpp       # WebSocket 观察者
├── WebSocketObserver.cpp
├── Logger.hpp                  # 日志记录器
├── Logger.cpp
── GlobalDefinition.hpp        # 全局定义
├── font.qrc                    # 字体资源
├── image.qrc                   # 图片资源
├── style.qrc                   # 样式表资源
├── style_light.qss             # 亮色主题样式
├── style_dark.qss              # 深色主题样式
└── TaskDispatcher.pro          # 项目配置文件
```

## 🚀 快速开始

### 环境要求

- Qt 5.12+
- C++11 编译器
- Windows / Linux / macOS

### 编译运行

#### 使用 Qt Creator

1. 打开 `TaskDispatcher.pro` 文件
2. 选择 Kit（建议使用 Qt 5.15+）
3. 点击 "Run" 按钮即可运行

#### 使用命令行

```bash
# 进入项目目录
cd TaskDispatcher

# 生成 Makefile
qmake TaskDispatcher.pro

# 编译
make

# 运行
./TaskDispatcher    # Linux/macOS
TaskDispatcher.exe  # Windows
```

##  主题切换

应用支持亮色和深色两种主题，可在菜单中切换：

- **视图 → 暗色主题**：启用/禁用深色模式

主题特点：
- macOS 风格渐变按钮
- 统一的深蓝色配色 (#007AFF)
- 圆角设计和阴影效果
- 流畅的交互反馈

## ⚙️ 主要功能说明

### 1. 任务管理

- **今日任务列表**：显示当天待执行的任务
- **任务进度追踪**：实时显示任务完成状态
- **自动刷新**：到达设定时间后自动刷新每日任务
- **手动添加**：支持临时添加一次性任务

### 2. 通知渠道

- **邮箱通知**：配置 SMTP 服务器发送任务提醒
- **企业微信**：通过企业微信机器人推送消息
- **测试功能**：可即时测试通知渠道是否正常工作

### 3. WebSocket 服务

- **启动服务**：选择本地 IP 地址启动 WebSocket 服务端
- **端口配置**：默认监听指定端口（可在代码中修改）
- **状态监控**：实时显示服务运行状态
- **数据接收**：接收并处理客户端发送的消息

### 4. 高级设置

- **任务超时时间**：设置任务执行的最大时长
- **任务重置时间**：自定义每日任务重置时间点
- **随机时间**：为任务添加随机延迟，避免集中执行
- **跳过节假日**：自动识别并跳过法定节假日

### 5. 数据管理

- **导入数据**：从外部文件导入任务配置
- **导出数据**：将当前配置导出备份
- **同步节假日**：从网络获取最新节假日数据

## 📝 开发指南

### 添加新通知渠道

1. 在 `MainWindow` 中添加新的配置项
2. 实现通知发送逻辑
3. 在菜单中添加对应设置项

### 修改 WebSocket 端口

编辑相关代码，修改端口号配置：

```cpp
// 示例：修改为 8080 端口
server->listen(QHostAddress::Any, 8080);
```

### 自定义样式

编辑 `style_light.qss` 或 `style_dark.qss` 文件：

```css
/* 修改按钮颜色 */
QPushButton {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #NEW_COLOR_1, 
                                stop:1 #NEW_COLOR_2);
}
```

## 🔧 常见问题

### Q: 如何切换主题？
A: 点击菜单栏 **视图 → 暗色主题** 即可切换。

### Q: WebSocket 服务无法启动？
A: 请检查：
- 端口是否被占用
- 防火墙是否阻止
- 选择的 IP 地址是否正确

### Q: 通知发送失败？
A: 请检查：
- 邮箱 SMTP 配置是否正确
- 企业微信机器人地址是否有效
- 网络连接是否正常

### Q: 节假日数据同步失败？
A: 请确保：
- 网络连接正常
- 节假日 API 服务可用
- 尝试稍后重新同步

##  许可证

本项目仅供学习和研究使用。

## 👥 贡献

欢迎提交 Issue 和 Pull Request！

## 📮 联系方式

如有问题或建议，欢迎反馈。

---

**注意**: 本项目的部分功能（如节假日 API、邮件服务等）需要配置相应的账号和密钥才能正常使用。
