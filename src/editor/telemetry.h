#pragma once

#include "hash_table.h"

enum Tm_Bits : u16 {
    TM_OPEN_BIT  = 0x1,
    TM_PAUSE_BIT = 0x2,
};

enum Tm_Sort_Type : u16 {
    TM_SORT_NONE,
    TM_SORT_NAME,
    TM_SORT_EXC,
    TM_SORT_INC,
    TM_SORT_CNT,
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

    u16 bits = 0;
    u16 sort_type = 0;

    u16 zone_total_count  = 0;
    u16 zone_active_count = 0;
    
    Tm_Zone *zones = null;
};

struct Window_Event;

void tm_init();
void tm_open();
void tm_close();
void tm_draw();
void tm_on_input(const Window_Event &event);
void tm_push_zone(String name);
void tm_pop_zone();
