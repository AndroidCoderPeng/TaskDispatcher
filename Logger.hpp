#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdarg>
#include <string>
#include <vector>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  static Logger Tag(const char *tag) { return Logger(tag); }

  // ========== 简单单行边框日志 ==========
  void d(const char *msg) const;

  void i(const char *msg) const;

  void w(const char *msg) const;

  void e(const char *msg) const;

  // ========== 带格式的边框日志 ==========
  template <typename... Args> void dFmt(const char *fmt, Args... args) {
    format(LogLevel::DEBUG, fmt, args...);
  }

  template <typename... Args> void iFmt(const char *fmt, Args... args) {
    format(LogLevel::INFO, fmt, args...);
  }

  template <typename... Args> void wFmt(const char *fmt, Args... args) {
    format(LogLevel::WARN, fmt, args...);
  }

  template <typename... Args> void eFmt(const char *fmt, Args... args) {
    format(LogLevel::ERROR, fmt, args...);
  }

  // ========== 多行内容边框日志（流式API） ==========
  // 使用方式: box().add("行1").add("行2").print();
  class BoxBuilder {
  public:
    BoxBuilder(Logger &logger, const LogLevel level)
        : logger(logger), level(level) {}

    // 添加一行内容
    BoxBuilder &add(const std::string &line);

    // 添加多行内容（针对sdp或者xml这种带有换行的内容块）
    BoxBuilder &addBlock(const std::string &content);

    // 添加格式化行
    __attribute__((format(printf, 2, 3))) BoxBuilder &addFmt(const char *fmt,
                                                             ...) {
      char buffer[256];
      va_list args;
      va_start(args, fmt);
      vsnprintf(buffer, sizeof(buffer), fmt, args);
      va_end(args);
      return add(buffer);
    }

    // 打印到logcat
    void print() const;

  private:
    Logger &logger;
    LogLevel level;
    std::vector<std::string> lines;
  };

  // 构建多行边框
  BoxBuilder box(LogLevel level = LogLevel::INFO);

  BoxBuilder dBox() { return box(LogLevel::DEBUG); }

  BoxBuilder iBox() { return box(LogLevel::INFO); }

  BoxBuilder wBox() { return box(LogLevel::WARN); }

  BoxBuilder eBox() { return box(LogLevel::ERROR); }

private:
  const char *tagPtr;
  static constexpr auto TAG_MAX_WIDTH = 20; // 日志标签最大宽度，超过会被截断
  static constexpr auto DEFAULT_WIDTH = 64; // 包括边框和空格在内的总宽度
  static constexpr auto H_LINE = "─";
  static constexpr auto V_LINE = "│";
  static constexpr auto TOP_LEFT = "┌";
  static constexpr auto TOP_RIGHT = "┐";
  static constexpr auto BOTTOM_LEFT = "└";
  static constexpr auto BOTTOM_RIGHT = "┘";

  explicit Logger(const char *tag) { tagPtr = tag; }

  /**
   * 绘制边框
   * @param left 左边框
   * @param fill 填充
   * @param right 右边框
   * @param level 日志级别
   */
  void drawBorder(const char *left, const char *fill, const char *right,
                  LogLevel level) const;

  /**
   * 打印内容
   * @param content 内容
   * @param level 日志级别
   */
  void printContent(const std::string &content, LogLevel level) const;

  /**
   * 格式化日志行
   * @param level 日志级别
   * @param msg 日志内容
   */
  void formatLogLine(LogLevel level, const char *msg) const;

  /**
   * 打印带边框的日志
   * @param level 日志级别
   * @param content 日志内容
   */
  void printStyledLog(LogLevel level, const char *content) const;

  template <typename... Args>
  void format(const LogLevel level, const char *fmt, Args... args) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), fmt, args...);
    printStyledLog(level, buffer);
  }
};

#endif // LOGGER_HPP
