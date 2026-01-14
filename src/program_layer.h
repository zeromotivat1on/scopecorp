#pragma once

#include "hash_table.h"

enum Program_Layer_Type : u8 {
    PROGRAM_LAYER_TYPE_NONE,
    PROGRAM_LAYER_TYPE_BASE,
    PROGRAM_LAYER_TYPE_GAME,
    PROGRAM_LAYER_TYPE_EDITOR,
    PROGRAM_LAYER_TYPE_CONSOLE,
    PROGRAM_LAYER_TYPE_PROFILER,
};

enum Key_Code : u8;

struct Window_Event;

struct Input_Mapping {
    void (*on_press  )();
    void (*on_release)();
    void (*on_repeat )();
};

struct Program_Layer {
    Program_Layer_Type               type;
    void                           (*on_push )(Program_Layer *);
    void                           (*on_pop  )(Program_Layer *);
    bool                           (*on_event)(Program_Layer *, const Window_Event *);
    Table <Key_Code, Input_Mapping> input_mapping_table;
};

struct Program_Layer_To_Process {
    Program_Layer *layer;
    bool           push;
};

void on_push_base    (Program_Layer *layer);
void on_push_game    (Program_Layer *layer);
void on_push_editor  (Program_Layer *layer);
void on_pop_base     (Program_Layer *layer);
void on_pop_game     (Program_Layer *layer);
void on_pop_editor   (Program_Layer *layer);
bool on_event_base   (Program_Layer *layer, const Window_Event *e);
bool on_event_game   (Program_Layer *layer, const Window_Event *e);
bool on_event_editor (Program_Layer *layer, const Window_Event *e);

// General event handler that searches input mapping by key and invokes
// appropriate function based on input event type.
bool on_event_input_mapping(Program_Layer *layer, const Window_Event *e);

inline Program_Layer program_layer_base;
inline Program_Layer program_layer_game;
inline Program_Layer program_layer_editor;
inline Program_Layer program_layer_console;
inline Program_Layer program_layer_profiler;

inline Array <Program_Layer_To_Process> program_layers_to_process;
inline Array <Program_Layer *>          program_layer_stack;

template <> u64 table_hash_proc(const Key_Code &a) { return hash_pcg(a); }

inline void push_program_layer(Program_Layer *to_push) {
    array_add(program_layers_to_process, { to_push, true });
}

inline Program_Layer *pop_program_layer() {
    auto layer = array_last(program_layer_stack);
    array_add(program_layers_to_process, { layer, false });
    return layer;
}

inline Program_Layer *get_program_layer() {
    return array_last(program_layer_stack);
}

inline void process_program_layer_stack() {
    For (program_layers_to_process) {
        if (it.push) {
            auto layer = array_add(program_layer_stack, it.layer);
            if (layer->on_push) layer->on_push(layer);
            else log(LOG_WARNING, "Program layer 0x%X has null on_push handler", layer);
        } else {
            auto layer = array_pop(program_layer_stack);
            Assert(it.layer == layer);
            if (layer->on_pop) layer->on_pop(layer);
            else log(LOG_WARNING, "Program layer 0x%X has null on_pop handler", layer);
        }
    }

    array_clear(program_layers_to_process);
}

inline void send_event(Program_Layer *layer, const Window_Event *e) {
    if (layer->on_event) layer->on_event(layer, e);
    else log(LOG_WARNING, "Program layer 0x%X has null on_event handler", layer);
}

inline void send_event_to_current_program_layer(const Window_Event *e) {
    auto layer = get_program_layer();
    send_event(layer, e);
}

inline void send_event_to_program_layer_stack(const Window_Event *e) {
    For (program_layer_stack) send_event(it, e);
}

inline String to_string(Program_Layer_Type type) {
    static const String lut[] = {
        S("none"), S("base"), S("game"), S("editor"), S("console"), S("profiler"),
    };
    return lut[type];
}
