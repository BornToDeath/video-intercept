//
// Created by lixiaoqing on 2021/11/17.
//

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <chrono>
#include <sstream>
#include "Log.h"

#define LOG_SIZE (1024)

/** 定义日志输出到标准输出时的颜色 */
#define LOG_COLOR_RESET  "\033[0m"
#define LOG_COLOR_RED    "\033[31m"
#define LOG_COLOR_PURPLE "\033[35m"

const char LogName[4] = {'D', 'I', 'W', 'E'};

enum class LogLevel : int {
    Debug = 0,
    Info,
    Warn,
    Error
};

inline void printLog(LogLevel level, const char *const tag, const char *const log) {
    const char logName = LogName[static_cast<std::underlying_type<LogLevel>::type>(level)];
    auto dateTime = Log::getCurrentDateTime();

    std::ostringstream oss;
    oss << dateTime
        << " | " << logName
        << " | " << tag
        << " | " << log;

    std::string logText(oss.str());
    switch (level) {
        case LogLevel::Error: {
            logText = LOG_COLOR_RED + logText + LOG_COLOR_RESET;
            break;
        }
        case LogLevel::Warn: {
            logText = LOG_COLOR_PURPLE + logText + LOG_COLOR_RESET;
            break;
        }
        default:
            break;
    }
    printf("%s\n", logText.c_str());
}

void Log::debug(const char *const tag, const char *const msg, ...) {
    char log[LOG_SIZE] = {};
    va_list arg_list;
    va_start(arg_list, msg);
    vsnprintf(log, sizeof(log), msg, arg_list);
    va_end(arg_list);
    printLog(LogLevel::Debug, tag, log);
}

void Log::info(const char *const tag, const char *const msg, ...) {
    char log[LOG_SIZE] = {};
    va_list arg_list;
    va_start(arg_list, msg);
    vsnprintf(log, sizeof(log), msg, arg_list);
    va_end(arg_list);
    printLog(LogLevel::Info, tag, log);
}

void Log::warn(const char *const tag, const char *const msg, ...) {
    char log[LOG_SIZE] = {};
    va_list arg_list;
    va_start(arg_list, msg);
    vsnprintf(log, sizeof(log), msg, arg_list);
    va_end(arg_list);
    printLog(LogLevel::Warn, tag, log);
}

void Log::error(const char *const tag, const char *const msg, ...) {
    char log[LOG_SIZE] = {};
    va_list arg_list;
    va_start(arg_list, msg);
    vsnprintf(log, sizeof(log), msg, arg_list);
    va_end(arg_list);
    printLog(LogLevel::Error, tag, log);
}

std::string Log::getCurrentDateTime() {
    constexpr int SIZE = 64;
    char now[SIZE] = {};
    auto curTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", std::localtime(&curTime));
    return {now};
}

Timestamp Log::getCurrentTimeMills() {
    auto mills = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    return mills;
}
