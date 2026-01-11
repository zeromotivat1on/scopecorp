#pragma once

#include "hash_table.h"

// @Todo: handle more types.
enum Constant_Type : u8 {
    CT_VOID,
    CT_S32,
    CT_F32,
    CT_VECTOR2,
    CT_VECTOR3,
    CT_VECTOR4,
    CT_MAT4,
    CT_CUSTOM, // user-defined type
    CT_ERROR   // result of trying to get unsupported constant type from slang alternative. @See slang::ScalarType.
};

struct Constant {
    static constexpr u32 type_sizes[] = { 0, 4, 4, 8, 12, 16, 64, 0, 0 };
    
    String name;
    Constant_Type type = CT_VOID;
    u16 count  = 0;
    u32 offset = 0; // local offset in constant buffer
    u32 size   = 0; // count * size of type
};

// Representation of constant buffer in shader, does not store any actual data,
// see Constant_Buffer_Instance for submitting to gpu.
struct Constant_Buffer {    
    String name;
    u32 size    = 0;
    u16 binding = 0;
    Table <String, Constant> constant_table;
};

struct Constant_Value {
    Constant *constant = null;
    void *data = null;
};

// Represents shader cbuffer and reflects its contents by storing them on cpu,
// submission to gpu is recorded by adding cbuffer instance to Command_Buffer
// which then uploads instance data to gpu using submit buffer.
struct Constant_Buffer_Instance {
    Constant_Buffer *constant_buffer = null;
    Table <String, Constant_Value> value_table;
};

Constant_Buffer *new_constant_buffer (String name, u32 size, u16 binding, u16 constant_count);
Constant_Buffer *get_constant_buffer (String name);
Constant        *add_constant        (Constant_Buffer *cb, String name, Constant_Type type, u16 count, u32 offset, u32 size);

Constant_Buffer_Instance make_constant_buffer_instance (Constant_Buffer *cb);
void set_constant       (Constant_Buffer_Instance *cbi, String name, const void *data, u32 size);
void set_constant_value (Constant_Value *cv, const void *data, u32 size);

template <typename T>
void set_constant(Constant_Buffer_Instance *cbi, String name, const T &data) {
    set_constant(cbi, name, &data, sizeof(data));
}

template <typename T>
void set_constant_value(Constant_Value *cv, const T &data) {
    set_constant_value(cv, &data, sizeof(data));
}

// Persistent global cbuffers.

inline Constant_Buffer_Instance cbi_global_parameters;
inline Constant_Value *cv_viewport_cursor_pos = null;
inline Constant_Value *cv_viewport_resolution = null;
inline Constant_Value *cv_viewport_ortho      = null;
inline Constant_Value *cv_camera_position     = null;
inline Constant_Value *cv_camera_view         = null;
inline Constant_Value *cv_camera_proj         = null;
inline Constant_Value *cv_camera_view_proj    = null;

inline Constant_Buffer_Instance cbi_level_parameters;
inline Constant_Value *cv_direct_light_count = null;
inline Constant_Value *cv_point_light_count  = null;
inline Constant_Value *cv_direct_lights      = null;
inline Constant_Value *cv_point_lights       = null;

inline Constant_Buffer_Instance cbi_frame_buffer_constants;
inline Constant_Value *cv_fb_transform                = null;
inline Constant_Value *cv_resolution                  = null;
inline Constant_Value *cv_pixel_size                  = null;
inline Constant_Value *cv_curve_distortion_factor     = null;
inline Constant_Value *cv_chromatic_aberration_offset = null;
inline Constant_Value *cv_quantize_color_count        = null;
inline Constant_Value *cv_noise_blend_factor          = null;
inline Constant_Value *cv_scanline_count              = null;
inline Constant_Value *cv_scanline_intensity          = null;
