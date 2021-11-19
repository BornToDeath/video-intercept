//
// Created by lixiaoqing on 2021/11/17.
//

#ifndef VIDEOINTERCEPT_LOG_H
#define VIDEOINTERCEPT_LOG_H

#include <iostream>
#include <string>
#include "Define.h"

class Log final {

public:

    Log() = delete;

    ~Log() = delete;

    static void debug(const char *const tag, const char *const msg, ...);

    static void info(const char *const tag, const char *const msg, ...);

    static void warn(const char *const tag, const char *const msg, ...);

    static void error(const char *const tag, const char *const msg, ...);

    static Timestamp getCurrentTimeMills();

    static std::string getCurrentDateTime();
};

#endif //VIDEOINTERCEPT_LOG_H
