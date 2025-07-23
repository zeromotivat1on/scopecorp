#pragma once

#include "math/vector.h"

inline s32 R_MAX_UNIFORM_BUFFER_BINDINGS;
inline s32 R_UNIFORM_BUFFER_BASE_ALIGNMENT;
inline s32 R_UNIFORM_BUFFER_OFFSET_ALIGNMENT;
inline s32 R_MAX_UNIFORM_BLOCK_SIZE;

inline constexpr s32 UNIFORM_BLOCK_BINDING_CAMERA        = 0;
inline constexpr s32 UNIFORM_BLOCK_BINDING_VIEWPORT      = 1;
inline constexpr s32 UNIFORM_BLOCK_BINDING_DIRECT_LIGHTS = 2;
inline constexpr s32 UNIFORM_BLOCK_BINDING_POINT_LIGHTS  = 3;

inline constexpr s32 UNIFORM_BLOCK_BINDINGS[] = {
    UNIFORM_BLOCK_BINDING_CAMERA,
    UNIFORM_BLOCK_BINDING_VIEWPORT,
    UNIFORM_BLOCK_BINDING_DIRECT_LIGHTS,
    UNIFORM_BLOCK_BINDING_POINT_LIGHTS,
};

inline const char *UNIFORM_BLOCK_NAME_CAMERA        = "Camera";
inline const char *UNIFORM_BLOCK_NAME_VIEWPORT      = "Viewport";
inline const char *UNIFORM_BLOCK_NAME_DIRECT_LIGHTS = "Direct_Lights";
inline const char *UNIFORM_BLOCK_NAME_POINT_LIGHTS  = "Point_Lights";

inline const char *UNIFORM_BLOCK_NAMES[] = {
    UNIFORM_BLOCK_NAME_CAMERA,
    UNIFORM_BLOCK_NAME_VIEWPORT,
    UNIFORM_BLOCK_NAME_DIRECT_LIGHTS,
    UNIFORM_BLOCK_NAME_POINT_LIGHTS,
};

inline constexpr s32 MAX_UNIFORM_NAME_SIZE = 64;
inline constexpr s32 MAX_UNIFORM_BUFFER_BLOCKS = 32;
inline constexpr s32 MAX_UNIFORM_BLOCK_FIELDS  = 16;

inline constexpr u32 MAX_UNIFORM_BUFFER_SIZE      = KB(16);
inline constexpr u32 MAX_UNIFORM_VALUE_CACHE_SIZE = KB(16);

inline rid RID_UNIFORM_BUFFER = RID_NONE;
inline u32 UNIFORM_BUFFER_SIZE = 0;

struct R_Uniform {
    sid name = SID_NONE;
	u16 type = 0;
	u16 count = 0;
    u32 size = 0;
    u32 offset = 0;
};

struct Uniform {
    sid sid_name = SID_NONE;
	u16 type = 0;
	u16 count = 0;
};

struct Uniform_Block_Field {
    u16 type = 0;
	u16 count = 1;
};

struct Uniform_Block {
    rid rid_uniform_buffer = RID_NONE;
	const char *name = null;
    Uniform_Block_Field fields[MAX_UNIFORM_BLOCK_FIELDS];
    s32 field_count = 0;
    s32 shader_binding = INVALID_INDEX; // must be unique for each block
    u32 offset = 0; // gpu aligned offset in uniform buffer
    u32 size   = 0; // gpu aligned size
};

struct Uniform_Value_Cache {
    void *data   = null;
    u32 size     = 0;
    u32 capacity = 0;
};

inline Uniform_Value_Cache uniform_value_cache;

inline Uniform_Block uniform_block_camera;
inline Uniform_Block uniform_block_viewport;
inline Uniform_Block uniform_block_direct_lights;
inline Uniform_Block uniform_block_point_lights;

u16  r_create_uniform(sid name, u16 type, u16 count);
void r_set_uniform(u16 uniform, u32 offset, u32 size, const void *data);

rid r_create_uniform_buffer(u32 size);
void r_add_uniform_block(rid rid_uniform_buffer, s32 shader_binding, const char *name, const Uniform_Block_Field *fields, s32 field_count, Uniform_Block *block);
void r_set_uniform_block_value(Uniform_Block *block, s32 field_index, s32 field_element_index, const void *data, u32 size);
void r_set_uniform_block_value(Uniform_Block *block, u32 offset, const void *data, u32 size);

u32 r_uniform_type_size(u16 type);
u32 r_uniform_type_dimension(u16 type);
u32 r_uniform_type_alignment(u16 type);
u32 r_uniform_type_size_gpu_aligned(u16 type);

u32 r_uniform_block_field_size(const Uniform_Block_Field &field);
u32 r_uniform_block_field_size_gpu_aligned(const Uniform_Block_Field &field);
u32 r_uniform_block_field_offset_gpu_aligned(Uniform_Block *block, s32 field_index, s32 field_element_index);
