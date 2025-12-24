#pragma once

enum Input_Layer_Type : u8 {
    INPUT_LAYER_GAME,
    INPUT_LAYER_EDITOR,
    INPUT_LAYER_CONSOLE,
    INPUT_LAYER_PROFILER,

    INPUT_LAYER_TYPE_COUNT
};

struct Input_Layer {
    Input_Layer_Type type = INPUT_LAYER_TYPE_COUNT;
    void (*on_input)(const struct Window_Event *e) = null;
    void (*on_push )() = null;
    void (*on_pop  )() = null;
};

struct Input_Stack {
    static constexpr u32 MAX_COUNT = 16;
    
    Input_Layer layers[MAX_COUNT];
    u32 layer_count = 0;
};

inline Input_Stack input_stack;
inline Input_Layer input_layers[INPUT_LAYER_TYPE_COUNT];

inline Input_Layer *get_current_input_layer() {
    if (input_stack.layer_count > 0) {
        return &input_stack.layers[input_stack.layer_count - 1];
    }
    return null;
}

inline Input_Layer_Type get_current_input_layer_type() {
    const auto *layer = get_current_input_layer();
    if (!layer) return INPUT_LAYER_TYPE_COUNT;
    return layer->type;
}

inline void push_input_layer(const Input_Layer &new_layer) {
    Assert(input_stack.layer_count < input_stack.MAX_COUNT);
    auto &layer = input_stack.layers[input_stack.layer_count] = new_layer;
    input_stack.layer_count += 1;
    if (layer.on_push) layer.on_push();
}

inline Input_Layer *pop_input_layer() {
    if (input_stack.layer_count == 0) return null;
    auto &layer = input_stack.layers[input_stack.layer_count - 1];
    input_stack.layer_count -= 1;
    if (layer.on_pop) layer.on_pop();
    return &layer;
}
