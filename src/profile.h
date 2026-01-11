#pragma once

#include "input.h"
#include "cpu_time.h"

#define TIMER_NAME(name)     __concat(_timer_, name)
#define START_TIMER(name)    const auto TIMER_NAME(name) = get_perf_counter()
#define CHECK_TIMER(name)    (get_perf_counter() - TIMER_NAME(name)) / (f32)get_perf_hz()
#define CHECK_TIMER_MS(name) (get_perf_counter() - TIMER_NAME(name)) / (f32)get_perf_hz_ms()

#define SCOPE_TIMER(info) const Scope_Timer TIMER_NAME(__LINE__)(info)
struct Scope_Timer {
    const char *info = null;
	u64 start = 0;
    
	Scope_Timer(const char *info) : info(info), start(get_perf_counter()) {}
	~Scope_Timer() { log("%s %.2fms", info, (f32)(get_perf_counter() - start) / get_perf_hz_ms()); }
};

inline constexpr auto KEY_OPEN_PROFILER        = KEY_F5;
inline constexpr auto KEY_SWITCH_PROFILER_VIEW = KEY_V;

enum Profiler_View_Mode : u8 {
    PROFILER_VIEW_RUNTIME,
    PROFILER_VIEW_MEMORY,
    PROFILER_VIEW_COUNT
};

inline constexpr u32 MAX_PROFILE_ZONES     = 256;
inline constexpr u32 MAX_CHILDREN_PER_ZONE = MAX_PROFILE_ZONES / 4;

// struct Profile_Sample {
//     s64 timestamp = 0;
//     Array <String> callstack;
// };

// struct Profile_Event {
//     enum Type { STARTED, FINISHED };
    
//     String name;
//     s64 timestamp = 0;
//     Type type;
// };

// @Todo: current limitation is that all zones must have unique names.
struct Profiler {
    static constexpr auto UPDATE_INTERVAL = 0.0f;
    
    struct Time_Zone {
        String name;
        u32    depth     = 0;
        s32    calls     = 0;
        s32    parent    = INDEX_NONE;        
        s64    start     = 0;
        s64    exclusive = 0;
        s64    inclusive = 0;
        s64    children  = 0;
    };
    
    f32                         update_time = 0.0f;
    f32                         max_graph_zone_time = 0.4f;
    s32                         selected_zone_index = 0;
    u8                          time_sort = 0;
    bool                        opened = false;
    bool                        paused = false;
    Profiler_View_Mode          view_mode;
    Array <Time_Zone>           all_zones;
    Array <Time_Zone *>         active_zones;
    Table <String, Time_Zone *> zone_lookup;
};

void      draw_dev_stats         ();
Profiler *get_profiler           ();
void      init_profiler          ();
void      open_profiler          ();
void      close_profiler         ();
void      update_profiler        ();
void      switch_profiler_view   ();
void      on_profiler_input      (const struct Window_Event *e);
void      push_profile_time_zone (const char *name);
void      push_profile_time_zone (String name);
void      pop_profile_time_zone  ();

#define Profile_Zone(x) push_profile_time_zone(x); defer { pop_profile_time_zone(); };
