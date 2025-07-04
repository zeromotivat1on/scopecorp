#pragma once

inline constexpr u32 MAX_INPUT_STACK_LAYERS = 16;

enum Input_Layer_Type : u8 {
    INPUT_LAYER_NONE,
    INPUT_LAYER_GAME,
    INPUT_LAYER_EDITOR,
    INPUT_LAYER_DEBUG_CONSOLE,
    INPUT_LAYER_RUNTIME_PROFILER,
    INPUT_LAYER_MEMORY_PROFILER,
};

typedef void(* On_Input)(struct Window_Event *event);

struct Input_Layer {
    On_Input on_input = null;
    Input_Layer_Type type = INPUT_LAYER_NONE;
};

struct Input_Stack {
    Input_Layer *layers[MAX_INPUT_STACK_LAYERS];
    u32 layer_count = 0;
};

inline Input_Stack input_stack;

inline Input_Layer input_layer_game;
inline Input_Layer input_layer_editor;
inline Input_Layer input_layer_debug_console;
inline Input_Layer input_layer_runtime_profiler;
inline Input_Layer input_layer_memory_profiler;

inline Input_Layer *get_current_input_layer() {
    if (input_stack.layer_count > 0) {
        return input_stack.layers[input_stack.layer_count - 1];
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

inline void push_input_layer(Input_Layer *layer) {
    Assert(input_stack.layer_count < MAX_INPUT_STACK_LAYERS);
    input_stack.layers[input_stack.layer_count] = layer;
    input_stack.layer_count += 1;
}

inline Input_Layer *pop_input_layer() {
    Assert(input_stack.layer_count > 0);
    auto *layer = get_current_input_layer();
    input_stack.layer_count -= 1;
    return layer;
}
