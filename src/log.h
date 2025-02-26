#pragma once

enum Log_Level {
    LOG_LOG,
    LOG_WARN,
    LOG_ERROR,
};

void log(const char* format, ...);
void warn(const char* format, ...);
void error(const char* format, ...);
