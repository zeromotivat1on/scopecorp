#pragma once

#define SEC(ms)     (ms / 1000)
#define MIN(ms)     (SEC(ms) / 60)
#define HOUR(ms)    (MIN(ms) / 60)

s64 current_time_ms();
s64 time_since_sys_boot_ms();

s64 performance_counter();
s64 performance_frequency();
