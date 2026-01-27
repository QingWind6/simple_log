#pragma once

#ifdef F
#undef F
#endif
#ifdef B0
#undef B0
#endif
#ifdef B1
#undef B1
#endif
#ifdef B2
#undef B2
#endif
#ifdef B3
#undef B3
#endif
#ifdef B4
#undef B4
#endif
#ifdef B5
#undef B5
#endif
#ifdef B6
#undef B6
#endif
#ifdef B7
#undef B7
#endif
#ifdef B8
#undef B8
#endif
#ifdef B9
#undef B9
#endif
#ifdef B10
#undef B10
#endif
#ifdef B11
#undef B11
#endif
#ifdef B12
#undef B12
#endif
#ifdef B13
#undef B13
#endif
#ifdef B14
#undef B14
#endif
#ifdef B15
#undef B15
#endif

#define FMT_HEADER_ONLY

#include "core.h"
#include "format.h"
#include <functional>
#include <string_view>

enum class LogLevel { INFO, WARN, ERROR, DEBUG };

class SimpleLog {
public:
    using OutputCallback = std::function<void(std::string_view)>;
    using TimeCallback = std::function<uint32_t()>;
    using LockCallback = std::function<void()>; // 锁的回调类型

    // 配置函数
    static void setOutput(OutputCallback cb) { GetOutputCb() = cb; }
    static void setTime(TimeCallback cb) { GetTimeCb() = cb; }
    static void setColorEnabled(bool on) { GetColorEnabled() = on; }
    
    // 配置锁 (传入加锁和解锁的函数)
    static void setLock(LockCallback lock_cb, LockCallback unlock_cb) {
        GetLockCb() = lock_cb;
        GetUnlockCb() = unlock_cb;
    }

    template <typename... Args>
    static void log(LogLevel level, fmt::format_string<Args...> format, Args&&... args) {
        // --- 1. 自动加锁 (RAII) ---
        struct AutoLock {
            AutoLock() { if (GetLockCb()) GetLockCb()(); }
            ~AutoLock() { if (GetUnlockCb()) GetUnlockCb()(); }
        } lock_guard; 

        // --- 2. 格式化逻辑 ---
        fmt::memory_buffer buffer;

        // 时间戳
        if (GetTimeCb()) {
            uint32_t now_ms = GetTimeCb()();
            fmt::format_to(std::back_inserter(buffer), "[{:>9.3f}] ", now_ms / 1000.0f);
        }

        // 颜色与标签
        const bool use_color = GetColorEnabled();
        switch (level) {
            case LogLevel::INFO:  fmt::format_to(std::back_inserter(buffer), "{}[INFO] ",  use_color ? "\033[32m" : ""); break;
            case LogLevel::WARN:  fmt::format_to(std::back_inserter(buffer), "{}[WARN] ",  use_color ? "\033[33m" : ""); break;
            case LogLevel::ERROR: fmt::format_to(std::back_inserter(buffer), "{}[ERRO] ",  use_color ? "\033[31m" : ""); break;
            case LogLevel::DEBUG: fmt::format_to(std::back_inserter(buffer), "{}[DBUG] ",  use_color ? "\033[36m" : ""); break;
        }

        // 内容
        fmt::format_to(std::back_inserter(buffer), format, std::forward<Args>(args)...);
        
        // 结束符
        if (use_color) {
            fmt::format_to(std::back_inserter(buffer), "\033[0m");
        }
        fmt::format_to(std::back_inserter(buffer), "\n");

        // --- 3. 输出 ---
        if (GetOutputCb()) {
            GetOutputCb()(std::string_view(buffer.data(), buffer.size()));
        }
        
        // 函数结束，lock_guard 析构，自动调用 unlock
    }

    // Helpers
    template <typename... Args> static void info(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::INFO, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void warn(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::WARN, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void error(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::ERROR, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void debug(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...); }

private:
    static OutputCallback& GetOutputCb() { static OutputCallback cb; return cb; }
    static TimeCallback& GetTimeCb() { static TimeCallback cb; return cb; }
    // 存储锁回调
    static LockCallback& GetLockCb() { static LockCallback cb; return cb; }
    static LockCallback& GetUnlockCb() { static LockCallback cb; return cb; }
    static bool& GetColorEnabled() { static bool color_on = true; return color_on; }
};
