#pragma once

enum Tm_Bits : u8 {
    TM_OPEN_BIT  = 0x1,
    TM_PAUSE_BIT = 0x2,
};

enum Tm_Sort_Type : u8 {
    TM_SORT_NONE,
    TM_SORT_NAME,
    TM_SORT_EXC,
    TM_SORT_INC,
    TM_SORT_CNT,
};

enum Tm_Precision_Type : u8 {
    TM_MICRO_SECONDS,
    TM_MILI_SECONDS,
    TM_SECONDS,
};

struct Tm_Zone {
    String name;
    u32 calls = 0;
    u32 depth = 0;
    s64 start = 0;
    s64 exclusive = 0;
    s64 inclusive = 0;
};

struct Tm_Context {
    static constexpr u16 MAX_ZONES = 256;

    u8 bits = 0;
    u8 sort_type = 0;
    u8 precision_type = 0;
    
    u16 zone_total_count  = 0;
    u16 zone_active_count = 0;
    
    Tm_Zone *zones = null;
    u16 *index_stack = null;
};

struct Window_Event;

void telemetry_init();
void telemetry_open();
void telemetry_close();
void telemetry_draw();
void telemetry_on_input(const Window_Event &event);
void telemetry_push_zone(String name);
void telemetry_pop_zone();
