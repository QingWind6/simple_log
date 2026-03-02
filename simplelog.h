#pragma once

#include "core.h"
#include "fmt_wrapper.h"
#include "format.h"
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string_view>

#if defined(__GNUC__) || defined(__clang__)
#define SIMPLELOG_PRINTF_ATTR(fmt_idx, first_arg) __attribute__((format(printf, fmt_idx, first_arg)))
#else
#define SIMPLELOG_PRINTF_ATTR(fmt_idx, first_arg)
#endif

enum class LogLevel { INFO, WARN, ERROR, DEBUG, VERBOSE };

class SimpleLog {
public:
    using OutputCallback = std::function<void(std::string_view)>;
    using TimeCallback = std::function<uint32_t()>;
    using LockCallback = std::function<void()>;

    // 配置函数
    static void setOutput(OutputCallback cb) { GetOutputCb() = cb; }
    static void setTime(TimeCallback cb) { GetTimeCb() = cb; }
    static void setColorEnabled(bool on) { GetColorEnabled() = on; }
    static void setLevelTagEnabled(bool on) { GetLevelTagEnabled() = on; }
    static void setTimestampEnabled(bool on) { GetTimestampEnabled() = on; }
    static void setLogLevel(LogLevel level) { GetLogLevel() = level; }
    
    // 配置锁
    static void setLock(LockCallback lock_cb, LockCallback unlock_cb) {
        GetLockCb() = lock_cb;
        GetUnlockCb() = unlock_cb;
    }

    template <typename... Args>
    static void log(LogLevel level, fmt::format_string<Args...> format, Args&&... args) {
        if (!ShouldLog(level)) {
            return;
        }
        // --- 1. 自动加锁 ---
        struct AutoLock {
            AutoLock() { if (GetLockCb()) GetLockCb()(); }
            ~AutoLock() { if (GetUnlockCb()) GetUnlockCb()(); }
        } lock_guard; 

        // --- 2. 格式化逻辑 ---
        // fmt::memory_buffer buffer;
        fmt::basic_memory_buffer<char, 128> buffer;

        // 时间戳
        if (GetTimestampEnabled() && GetTimeCb()) {
            uint32_t now_ms = GetTimeCb()();
            fmt::format_to(std::back_inserter(buffer), "[{:>9.3f}] ", now_ms / 1000.0f);
        }

        // 颜色与标签
        const bool show_level = GetLevelTagEnabled();
        const bool use_color = GetColorEnabled();
        const char* color_code = "";
        const char* tag = "";
        switch (level) {
            case LogLevel::INFO:  color_code = "\033[32m"; tag = "[INFO] "; break;
            case LogLevel::WARN:  color_code = "\033[33m"; tag = "[WARN] "; break;
            case LogLevel::ERROR: color_code = "\033[31m"; tag = "[ERRO] "; break;
            case LogLevel::DEBUG: color_code = "\033[36m"; tag = "[DBUG] "; break;
            case LogLevel::VERBOSE: color_code = "\033[90m"; tag = "[VERB] "; break;
        }

        if (use_color) {
            fmt::format_to(std::back_inserter(buffer), "{}", color_code);
        }
        if (show_level) {
            fmt::format_to(std::back_inserter(buffer), "{}", tag);
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
    template <typename... Args> static void infof(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::INFO, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void warnf(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::WARN, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void errorf(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::ERROR, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void debugf(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...); }
    template <typename... Args> static void verbosef(fmt::format_string<Args...> fmt, Args&&... args) { log(LogLevel::VERBOSE, fmt, std::forward<Args>(args)...); }
    static void info(std::string_view msg) { log(LogLevel::INFO, "{}", msg); }
    static void warn(std::string_view msg) { log(LogLevel::WARN, "{}", msg); }
    static void error(std::string_view msg) { log(LogLevel::ERROR, "{}", msg); }
    static void debug(std::string_view msg) { log(LogLevel::DEBUG, "{}", msg); }
    static void verbose(std::string_view msg) { log(LogLevel::VERBOSE, "{}", msg); }
    static void info(std::string_view tag, std::string_view msg) { log(LogLevel::INFO, "[{}] {}", tag, msg); }
    static void warn(std::string_view tag, std::string_view msg) { log(LogLevel::WARN, "[{}] {}", tag, msg); }
    static void error(std::string_view tag, std::string_view msg) { log(LogLevel::ERROR, "[{}] {}", tag, msg); }
    static void debug(std::string_view tag, std::string_view msg) { log(LogLevel::DEBUG, "[{}] {}", tag, msg); }
    static void verbose(std::string_view tag, std::string_view msg) { log(LogLevel::VERBOSE, "[{}] {}", tag, msg); }

    // printf-style helpers for legacy call sites
    static void infoln(std::string_view msg) { log(LogLevel::INFO, "{}", msg); }
    static void warningln(std::string_view msg) { log(LogLevel::WARN, "{}", msg); }
    static void warnln(std::string_view msg) { log(LogLevel::WARN, "{}", msg); }
    static void errorln(std::string_view msg) { log(LogLevel::ERROR, "{}", msg); }
    static void fatalln(std::string_view msg) { log(LogLevel::ERROR, "{}", msg); }
    static void debugln(std::string_view msg) { log(LogLevel::DEBUG, "{}", msg); }
    static void verboseln(std::string_view msg) { log(LogLevel::VERBOSE, "{}", msg); }

    static void infoln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::INFO, format, args);
        va_end(args);
    }
    static void warningln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::WARN, format, args);
        va_end(args);
    }
    static void warnln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::WARN, format, args);
        va_end(args);
    }
    static void errorln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::ERROR, format, args);
        va_end(args);
    }
    static void fatalln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::ERROR, format, args);
        va_end(args);
    }
    static void debugln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::DEBUG, format, args);
        va_end(args);
    }
    static void verboseln(const char* format, ...) SIMPLELOG_PRINTF_ATTR(1, 2) {
        va_list args;
        va_start(args, format);
        vlogf(LogLevel::VERBOSE, format, args);
        va_end(args);
    }

private:
    static void vlogf(LogLevel level, const char* format, va_list args) {
        if (!format || !ShouldLog(level)) {
            return;
        }

        constexpr size_t kPrintfBufferSize = 320;
        std::array<char, kPrintfBufferSize> stack_buf{};

        va_list args_copy;
        va_copy(args_copy, args);
        const int written = std::vsnprintf(stack_buf.data(), stack_buf.size(), format, args_copy);
        va_end(args_copy);
        if (written < 0) {
            return;
        }

        size_t len = strnlen(stack_buf.data(), stack_buf.size());
        if (static_cast<size_t>(written) >= stack_buf.size() && stack_buf.size() > 4) {
            // Keep log path allocation-free: truncate long printf payloads.
            stack_buf[stack_buf.size() - 4] = '.';
            stack_buf[stack_buf.size() - 3] = '.';
            stack_buf[stack_buf.size() - 2] = '.';
            stack_buf[stack_buf.size() - 1] = '\0';
            len = stack_buf.size() - 1;
        }

        log(level, "{}", std::string_view(stack_buf.data(), len));
    }

    static OutputCallback& GetOutputCb() { static OutputCallback cb; return cb; }
    static TimeCallback& GetTimeCb() { static TimeCallback cb; return cb; }
    // 存储锁回调
    static LockCallback& GetLockCb() { static LockCallback cb; return cb; }
    static LockCallback& GetUnlockCb() { static LockCallback cb; return cb; }
    static bool& GetColorEnabled() { static bool color_on = true; return color_on; }
    static bool& GetLevelTagEnabled() { static bool level_tag_on = true; return level_tag_on; }
    static bool& GetTimestampEnabled() { static bool timestamp_on = true; return timestamp_on; }
    static LogLevel& GetLogLevel() { static LogLevel level = LogLevel::DEBUG; return level; }
    static bool ShouldLog(LogLevel level) { return LevelRank(level) >= LevelRank(GetLogLevel()); }
    static int LevelRank(LogLevel level) {
        switch (level) {
            case LogLevel::VERBOSE: return 0;
            case LogLevel::DEBUG: return 1;
            case LogLevel::INFO: return 2;
            case LogLevel::WARN: return 3;
            case LogLevel::ERROR: return 4;
        }
        return 0;
    }
};
