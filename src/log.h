#pragma once

enum Log_Level {
	LOG_LOG,
	LOG_WARN,
	LOG_ERROR,
    
    LOG_NONE // skip logging
};

void print(Log_Level level, const char *format, ...);
void log(const char *format, ...);
void warn(const char *format, ...);
void error(const char *format, ...);
