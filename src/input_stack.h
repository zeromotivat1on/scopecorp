#pragma once

enum Input_Layer_Type : u8 {
    INPUT_LAYER_NONE,
    INPUT_LAYER_GAME,
    INPUT_LAYER_EDITOR,
    INPUT_LAYER_DEBUG_CONSOLE,
    INPUT_LAYER_RUNTIME_PROFILER,
    INPUT_LAYER_MEMORY_PROFILER,
};

struct Window_Event;

typedef void(* On_Input)(const Window_Event &event);

struct Input_Layer {
    On_Input on_input = null;
    Input_Layer_Type type = INPUT_LAYER_NONE;
};

struct Input_Stack {
    static constexpr u32 MAX_COUNT = 16;
    
    Input_Layer layers[MAX_COUNT];
    u32 layer_count = 0;
};

inline Input_Stack Input_stack;

inline Input_Layer Input_layer_game;
inline Input_Layer Input_layer_editor;
inline Input_Layer Input_layer_debug_console;
inline Input_Layer Input_layer_runtime_profiler;
inline Input_Layer Input_layer_memory_profiler;

inline Input_Layer *get_current_input_layer() {
    if (Input_stack.layer_count > 0) {
        return &Input_stack.layers[Input_stack.layer_count - 1];
    }
    return null;
}

inline Input_Layer_Type get_current_input_layer_type() {
    const auto *layer = get_current_input_layer();
    if (layer) {
        return layer->type;
    }
    return INPUT_LAYER_NONE;
}

inline void push_input_layer(const Input_Layer &layer) {
    Assert(Input_stack.layer_count < Input_stack.MAX_COUNT);
    Input_stack.layers[Input_stack.layer_count] = layer;
    Input_stack.layer_count += 1;
}

inline Input_Layer &pop_input_layer() {
    Assert(Input_stack.layer_count > 0);
    auto &layer = Input_stack.layers[Input_stack.layer_count - 1];
    Input_stack.layer_count -= 1;
    return layer;
}
