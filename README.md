# TaskDispatcher - Android 设备定时任务调度器

一个基于 Qt 5 的跨平台桌面应用，用于自动化管理 Android 设备的定时任务。通过 ADB 命令在预设时间点唤醒 Android 设备、打开指定 App、截屏，并将结果通过邮件或企业微信通知用户。

## 功能特性

- **三层任务调度引擎**：预设时间点 → 随机时间偏移 → 节假日跳过，三层逐级过滤，每日自动循环
- **完整 ADB 交互链**：亮屏、滑动解锁、打开 App、截屏、杀进程、息屏全流程自动化
- **无线 ADB 连接**：支持 TCP/IP 无线连接 Android 设备，每 5 秒自动检测连接状态并显示设备品牌名
- **随机时间偏移**：任务执行时间可添加随机偏移（3-30 分钟），每次周期开始时预计算所有偏移量，过期任务自动跳过
- **节假日智能跳过**：自动同步中国法定节假日数据（含调休补班识别），多 CDN 源自动故障转移，按年份缓存
- **双重通知渠道**：QQ 邮箱（SMTP/SSL，smtp.qq.com:465）和企业微信 Webhook 机器人推送截图
- **截屏智能压缩**：阶梯降分辨率 PNG 编码（1920→1600→1280→1024→800），自动压缩至 2MB 以内
- **任务持久化**：SQLite 存储任务数据，JSON 文件存储配置，单例模式线程安全访问
- **数据导入导出**：支持配置和任务数据的 JSON 格式导入/导出，含版本号和导出时间戳
- **双主题支持**：亮色/暗色主题实时切换，主色调 `#007AFF`，QSS 样式表驱动
- **系统托盘**：支持最小化到系统托盘后台静默运行，关闭时可选择最小化或直接退出
- **Toast 消息提示**：非阻塞浮动消息（Info/Warning/Error 三级），自动消失，不干扰操作
- **美化日志系统**：控制台彩色 ANSI 输出，Unicode 表格字符边框美化，流式 API
- **内置帮助系统**：HTML 格式 FAQ 页面，涵盖 MIUI/HyperOS ADB 限制说明和无线 ADB 连接指南

## 系统要求

- **操作系统**：Windows / Linux / macOS
- **运行时环境**：Qt 5.12+
- **外部依赖**：Android ADB（Windows 下自动使用内置 `adb`）
- **编译标准**：C++14

## 构建依赖

| 依赖 | 说明 |
|------|------|
| Qt 5.12+ | 核心框架 |
| Qt Widgets | UI 组件 |
| Qt Network | 网络请求（HTTP/HTTPS） |
| Qt SQL | SQLite 数据库 |
| Qt Concurrent | 并发支持 |
| qmake | 构建系统 |

## 编译构建

```bash
# 使用 Qt Creator 打开 TaskDispatcher.pro
# 或命令行构建：
cd TaskDispatcher
qmake TaskDispatcher.pro
make          # Linux / macOS
mingw32-make  # Windows MinGW
```

### Windows 安装包

```bash
script/windows_x86_64/build_release.bat
```

该脚本自动执行 windeployqt、拷贝 MinGW 运行时库和 OpenSSL DLL、拷贝 ADB 工具，最终通过 Inno Setup 生成 `TaskDispatcher_Setup.exe`。

## 配置说明

| 配置项 | 默认值 | 范围 | 说明 |
|--------|--------|------|------|
| 目标 APP | 无 | 钉钉/企微/飞书/QQ/微信/自定义包名 | 自动流程中要打开的应用 |
| 通知方式 | 邮箱 | 邮箱 / 企业微信 | 选择通知渠道 |
| 邮件标题 | "指令执行结果通知" | 任意文本 | 邮件主题 |
| 发件邮箱 | 无 | QQ 邮箱 | SMTP 发件箱 |
| 授权码 | 无 | QQ 邮箱 SMTP 授权码 | SMTP 认证凭据 |
| 收件邮箱 | 无 | 任意邮箱 | 接收通知的邮箱 |
| 企微消息标题 | "指令执行结果通知" | 任意文本 | — |
| 企微 Key | 无 | 企业微信 Webhook Key | 消息推送凭据 |
| 任务等待时间 | 30 秒 | 10–120 秒 | 打开 App 后等待截屏的延迟 |
| 任务重置时间 | 00:00:00 | 任意时间 | 每日任务周期重置点 |
| 随机时间范围 | 5 分钟 | 3–30 分钟 | 任务执行时间随机偏移范围 |
| 随机任务开关 | 开启 | 开/关 | 是否启用随机时间偏移 |
| 跳过节假日 | 开启 | 开/关 | 法定节假日自动跳过 |
| 重置任务开关 | 开启 | 开/关 | 是否每日自动重置任务 |
| 暗色主题 | 关闭 | 开/关 | 亮/暗双主题切换 |

## 三层调度逻辑

```
第一层：预设时间点
  └→ 按用户设置的时间点顺序执行
第二层：随机时间偏移
  └→ 每次周期开始时预计算所有任务的随机偏移（3-30 分钟）
  └→ 过期任务自动跳过（考虑偏移最晚可能时间）
第三层：节假日检测
  └→ 开启时检查当天是否为法定节假日 → 是则整日跳过
```

## 执行流程

```
任务触发
  → 亮屏（adb shell input keyevent KEYCODE_WAKEUP）
  → 获取屏幕分辨率（wm size）
  → 解锁（底部 4/5 → 顶部 1/5 滑动手势）
  → 等待 3 秒稳定
  → 打开目标应用（adb shell monkey）
  → 等待 N 秒（可配置，默认 30 秒）
  → 截屏（adb exec-out screencap -p）→ 保存至 capture/ 目录
  → 压缩图片（阶梯降分辨率 PNG 编码，目标 ≤2MB）
  → 发送通知（QQ 邮箱附件 / 企业微信图片消息）
  → 等待 15 秒
  → 关闭目标应用（adb shell am force-stop）
  → 检查是否还有未执行任务 → 如有则调度下一个
  → 当日任务完成 → 等待至次日重置时间 → 开启新周期
```

## 菜单功能

| 菜单 | 功能项 |
|------|--------|
| **文件(F)** | 导入数据 `Ctrl+I`、导出数据 `Ctrl+E`、退出 `Ctrl+Q` |
| **设置(S)** | 目标 APP、邮箱配置、企业微信配置、任务等待时间 `Ctrl+T`、任务重置时间 `Ctrl+R`、随机时间范围、随机任务开关、跳过节假日开关、重置任务开关、暗色主题开关 |
| **数据(D)** | 同步节假日数据（含年份缓存） |
| **指令(C)** | 亮屏、截屏、息屏、打开目标应用、关闭目标应用、重启 ADB 服务 |
| **帮助(H)** | 测试邮箱通知、测试企业微信通知、常见问题（HTML 帮助页）、项目主页、关于 |

## 无线 ADB 连接

- 启动时自动检测 ADB 连接状态并周期性检测（每 5 秒）
- 断开时显示 USB 断开图标 + "未连接"，已连接显示 USB 图标 + 设备品牌名
- 点击"连接手机"按钮，输入设备 IP 地址进行 TCP/IP 无线连接
- 支持 ADB 服务重启
- 屏幕截图功能

## 架构概览

```
DispatcherApplication (QApplication)
  └── MainWindow (QMainWindow)
        ├── 菜单栏
        │     ├── 文件: 导入/导出/退出
        │     ├── 设置: 全部配置项
        │     ├── 数据: 节假日同步
        │     ├── 指令: ADB 快捷操作
        │     └── 帮助: 测试/FAQ/关于
        ├── 主界面
        │     ├── 日期/时间显示 + 倒计时
        │     ├── 执行/停止控制 + 通知方式切换
        │     ├── 任务进度指示
        │     └── 任务列表 (QListWidget + TaskItemWidget)
        ├── 底部状态栏
        │     └── USB 连接图标 + 连接状态 + 连接手机按钮
        └── 系统托盘
              └── 显示主窗口 / 退出

核心模块:
  ├── TaskExecutor         [三层调度引擎]
  │     └── TaskStore      [SQLite 任务存储]
  ├── ProcessExecutor      [ADB 命令执行器]
  │     ├── 亮屏/解锁/截屏/打开App/杀进程/息屏
  │     └── 无线连接/设备检测/设备名称获取
  ├── MailSender           [QQ 邮箱 SMTP/SSL]
  ├── WxMessageSender      [企业微信 Webhook]
  ├── ChinaHolidayManager  [节假日数据同步与查询]
  ├── ImageProcessor       [PNG 阶梯压缩]
  ├── ConfigStore          [JSON 配置持久化（线程安全单例）]
  ├── ToastWidget          [非阻塞消息提示]
  └── TaskItemWidget       [任务列表项渲染]
```

## 数据存储

| 存储对象 | 文件 | 说明 |
|----------|------|------|
| 任务数据 | `tasks.db`（SQLite） | 定时任务 CRUD，含 ID 和计划时间 |
| 配置数据 | `task_config.json`（JSON） | 邮件、企微、App、时间等全量配置 |
| 截图文件 | `capture/` 目录 | `yyyyMMdd_HHmmss.png` 格式命名 |
| 节假日缓存 | `task_config.json` 内 `holidayConfig` 节点 | 按年份缓存中国节假日数据 |
| 主题样式 | `style_light.qss` / `style_dark.qss` | QSS 样式表，`#007AFF` 蓝色主色调 |

## 节假日数据源

- **数据来源**：`chinese-days` npm 包
- **CDN 镜像**：`cdn.jsdelivr.net`（主）、`fastly.jsdelivr.net`、`registry.npmmirror.com`（备用）
- **容错机制**：多源自动故障转移，取首个成功响应
- **缓存策略**：按年份缓存至 ConfigStore，同一年份复用缓存，跨年自动更新

## 应用包名映射

| 应用 | 包名 |
|------|------|
| 钉钉 | `com.alibaba.android.rimet` |
| 企业微信 | `com.tencent.wework` |
| 飞书 | `com.ss.android.lark` |
| QQ | `com.tencent.mobileqq` |
| 微信 | `com.tencent.mm` |

> 配置时可直接输入中文名称（如"钉钉"），也可直接输入自定义包名。

## 项目结构

```
TaskDispatcher/
├── DispatcherApplication.hpp/.cpp    # QApplication 子类，加载字体/样式
├── GlobalDefinition.hpp              # 全局数据结构定义
├── ConfigStore.hpp/.cpp              # JSON 配置存储（线程安全单例）
├── ChinaHolidayManager.hpp/.cpp      # 中国节假日管理
├── AddTaskDialog.hpp/.cpp/.ui        # 添加/编辑任务对话框
├── EmailSettingDialog.hpp/.cpp/.ui   # 邮箱配置对话框
├── ImageProcessor.hpp/.cpp           # PNG 图片阶梯压缩
├── main.cpp                          # 程序入口
├── TaskDispatcher.pro                # qmake 项目文件
├── font.qrc / image.qrc              # 资源文件
├── html/index.html                   # 内置帮助页面
├── script/                           # 构建与部署脚本
│   └── windows_x86_64/
│       ├── build_release.bat         # 发布构建脚本
│       ├── TaskDispatcher.iss        # Inno Setup 安装脚本
│       └── README.txt                # 编译部署说明
└── tool/
    └── windows/                      # Windows ADB 工具
        ├── adb.exe
        ├── mke2fs.conf
        ├── NOTICE.txt
        └── source.properties
```

## 许可证

本项目仅供学习和个人使用。
