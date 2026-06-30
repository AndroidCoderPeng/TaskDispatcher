# TaskDispatcher - Android 设备定时任务调度器

一个基于 Qt 5 的跨平台桌面应用，用于自动化管理 Android 设备的定时任务。通过 ADB 命令在预设时间点唤醒 Android 设备、打开指定
App、截屏，并将结果通过邮件或企业微信通知用户。

## 功能特性

- **定时任务调度**：按预设时间点自动执行任务，支持每日循环
- **ADB设备交互**：亮屏、解锁、打开 App、截屏、杀进程、息屏全流程自动化
- **随机时间偏移**：任务执行时间可添加随机偏移（3-30分钟），避免规律性被检测
- **节假日跳过**：自动同步中国法定节假日数据，节假日自动跳过任务执行
- **双重通知渠道**：支持 QQ邮箱（SMTP/SSL）和企业微信 Webhook 机器人推送截图
- **截屏压缩**：自动压缩截屏 PNG 图片至 2MB 以内，满足邮件附件和企微图片大小限制
- **任务持久化**：SQLite 存储任务数据，JSON 文件存储配置
- **数据导入导出**：支持配置和任务数据的 JSON 格式导入/导出
- **双主题支持**：亮色/暗色主题一键切换
- **系统托盘**：支持最小化到系统托盘，后台静默运行
- **日志系统**：控制台彩色边框日志输出，方便调试和监控

## 截图预览

主界面包含：当前日期/时间显示、任务倒计时、执行/停止控制、通知方式选择、任务进度指示、任务列表管理等功能。

## 系统要求

- **操作系统**：Windows / Linux / macOS
- **运行时环境**：Qt 5.12+
- **外部依赖**：Android ADB（需在 PATH 环境变量中）
- **编译标准**：C++14

## 构建依赖

| 依赖         | 说明         |
|------------|------------|
| Qt 5.12+   | 核心框架       |
| Qt Widgets | UI 组件      |
| Qt Network | 网络请求       |
| Qt SQL     | SQLite 数据库 |
| qmake      | 构建系统       |

## 编译构建

```bash
# 使用 Qt Creator 打开 TaskDispatcher.pro
# 或命令行构建：
cd TaskDispatcher
qmake TaskDispatcher.pro
make          # Linux / macOS
mingw32-make  # Windows MinGW
```

## 配置说明

| 配置项    | 默认值      | 范围               | 说明                     |
|--------|----------|------------------|------------------------|
| 目标APP  | 无        | 钉钉/企微/飞书/自定义包名   | 自动流程中要打开的应用            |
| 邮箱配置   | 无        | QQ邮箱             | SMTP 发送参数（发件箱、授权码、收件箱） |
| 企微配置   | 无        | 企业微信 Webhook Key | 机器人消息推送                |
| 通知方式   | 邮箱       | 邮箱/企微            | 选择通知渠道                 |
| 任务等待时间 | 30秒      | 10-120秒          | 打开 App 后等待截屏的延迟        |
| 任务重置时间 | 00:00:00 | 任意时间             | 每日任务重置点                |
| 随机时间   | 开启，5分钟   | 3-30分钟           | 任务执行时间随机偏移范围           |
| 跳过节假日  | 开启       | 开/关              | 法定节假日自动跳过              |
| 暗色主题   | 关闭       | 开/关              | 亮/暗双主题切换               |

## 执行流程

```
任务触发
  → 亮屏（adb shell input keyevent KEYCODE_WAKEUP）
  → 解锁（滑动解锁手势）
  → 打开目标应用（adb shell monkey）
  → 等待 N 秒（可配置）
  → 截屏（adb exec-out screencap -p）
  → 压缩图片（阶梯降分辨率 PNG 编码，目标 ≤2MB）
  → 发送通知（邮件附件 / 企业微信图片消息）
  → 等待 15 秒
  → 关闭目标应用（adb shell am force-stop）
  → 息屏（adb shell input keyevent KEYCODE_SLEEP）
```

## 架构概览

```
DispatcherApplication (QApplication)
  └── MainWindow (QMainWindow)          [中央控制器]
        ├── TaskExecutor                [调度引擎]
        │     └── TaskStore             [SQLite 任务存储]
        ├── ProcessExecutor             [ADB 命令执行]
        ├── MailSender                  [邮件通知]
        ├── WxMessageSender             [企业微信通知]
        ├── ChinaHolidayManager         [节假日管理]
        ├── ImageProcessor              [图片压缩]
        ├── ConfigStore                 [JSON 配置存储]
        ├── ToastWidget                 [Toast 提示]
        └── TaskItemWidget              [任务列表项]
```

## 数据存储

- **任务数据**：SQLite 数据库（`tasks.db`），存储所有定时任务
- **配置数据**：JSON 文件（`task_config.json`），存储邮件、企微、App 等配置
- **截图文件**：`capture/` 目录，存储压缩前后的截屏图片
- **节假日缓存**：ConfigStore 中按年份缓存中国节假日数据

## 快捷操作

- `Ctrl+I` — 导入数据
- `Ctrl+E` — 导出数据
- `Ctrl+T` — 设置任务等待时间
- `Ctrl+R` — 设置任务重置时间
- `Ctrl+Q` — 退出程序

## 应用包名映射

| 应用   | 包名                        |
|------|---------------------------|
| 钉钉   | com.alibaba.android.rimet |
| 企业微信 | com.tencent.wework        |
| 飞书   | com.ss.android.lark       |
| QQ   | com.tencent.mobileqq      |
| 微信   | com.tencent.mm            |

## 许可证

本项目仅供学习和个人使用。
