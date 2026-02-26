# simple_log

轻量级 C++ 头文件日志库，基于 fmt 格式化，支持颜色、时间戳、锁、日志级别过滤。

## 特性
- 仅头文件，直接 include 即可使用
- 支持 INFO/WARN/ERROR/DEBUG/VERBOSE 五级
- 可设置全局输出回调、时间回调、锁回调
- 支持彩色输出、等级标签、时间戳开关

## 基本使用
```cpp
#include "simplelog.h"

SimpleLog::setOutput([](std::string_view s) {
    // 这里示例使用 printf，也可以写入文件/串口等
    printf("%.*s", (int)s.size(), s.data());
});

SimpleLog::info("hello");
SimpleLog::warnf("value = {}", 42);
```

## 日志级别过滤
设置最低输出级别，低于该级别的日志将被丢弃。
```cpp
SimpleLog::setLogLevel(LogLevel::ERROR);
SimpleLog::debug("dbg");   // 不输出
SimpleLog::error("err");   // 输出
```

等级顺序（低 -> 高）：
```
VERBOSE < DEBUG < INFO < WARN < ERROR
```

## 配置项
```cpp
// 输出回调（必须配置，才会实际输出）
SimpleLog::setOutput([](std::string_view s) { /* ... */ });

// 时间回调（毫秒）
SimpleLog::setTime([]() -> uint32_t { return 1234; });

// 互斥锁（可选）
SimpleLog::setLock([](){ /* lock */ }, [](){ /* unlock */ });

// 开关
SimpleLog::setColorEnabled(true);     // 颜色
SimpleLog::setLevelTagEnabled(true);  // [INFO] 标签
SimpleLog::setTimestampEnabled(true); // 时间戳
```

## ESP32 示例（含互斥锁与配置）
```cpp
SemaphoreHandle_t logMutex;

void setup_logging() {
    logMutex = xSemaphoreCreateMutex();

    SimpleLog::setLock(
        []() { xSemaphoreTake(logMutex, portMAX_DELAY); },
        []() { xSemaphoreGive(logMutex); }
    );

    // 输出与时间回调
    SimpleLog::setOutput([](std::string_view msg) {
        printf("%.*s", static_cast<int>(msg.size()), msg.data());
    });
    SimpleLog::setTime([]() { return (uint32_t)esp_timer_get_time() / 1000; }); // us -> ms
    SimpleLog::setLevelTagEnabled(false);
    SimpleLog::setLogLevel(LogLevel::ERROR);
}
```

## 便捷接口
```cpp
SimpleLog::info("msg");
SimpleLog::warn("msg");
SimpleLog::error("msg");
SimpleLog::debug("msg");
SimpleLog::verbose("msg");

SimpleLog::infof("x = {}", x);
SimpleLog::warnf("x = {}", x);
SimpleLog::errorf("x = {}", x);
SimpleLog::debugf("x = {}", x);
SimpleLog::verbosef("x = {}", x);
```
