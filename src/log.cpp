#include "pch.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void log_output_va(Log_Level log_level, const char* format, va_list args)
{
    const char* prefixes[] = {"\x1b[37m[LOG]:   ", "\x1b[93m[WARN]:  ", "\x1b[91m[ERROR]: "};
    
    char buffer[1024] = {0};
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s%s", prefixes[log_level], buffer);

    // Restore default bg and fg colors.
    puts("\x1b[39;49m");
}

void log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_output_va(LOG_LOG, format, args);
    va_end(args);
}

void warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_output_va(LOG_WARN, format, args);
    va_end(args);
}

void error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    log_output_va(LOG_ERROR, format, args);
    va_end(args);
}

