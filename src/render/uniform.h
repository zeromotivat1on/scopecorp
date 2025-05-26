#pragma once

#include "math/vector.h"

inline s32 R_MAX_UNIFORM_BUFFER_BINDINGS;
inline s32 R_UNIFORM_BUFFER_BASE_ALIGNMENT;
inline s32 R_UNIFORM_BUFFER_OFFSET_ALIGNMENT;

inline s32 R_MAX_UNIFORM_BLOCK_SIZE;

inline s32 UNIFORM_BLOCK_CAMERA;
inline s32 UNIFORM_BLOCK_VIEWPORT;
inline s32 UNIFORM_BLOCK_DIRECT_LIGHTS;
inline s32 UNIFORM_BLOCK_POINT_LIGHTS;

inline constexpr s32 UNIFORM_BINDING_CAMERA        = 0;
inline constexpr s32 UNIFORM_BINDING_VIEWPORT      = 1;
inline constexpr s32 UNIFORM_BINDING_DIRECT_LIGHTS = 2;
inline constexpr s32 UNIFORM_BINDING_POINT_LIGHTS  = 3;

inline const char *UNIFORM_BLOCK_NAME_CAMERA = "Camera";
inline const char *UNIFORM_BLOCK_NAME_VIEWPORT = "Viewport";
inline const char *UNIFORM_BLOCK_NAME_DIRECT_LIGHTS = "Direct_Lights";
inline const char *UNIFORM_BLOCK_NAME_POINT_LIGHTS  = "Point_Lights";

inline constexpr s32 MAX_UNIFORM_BUFFER_BLOCKS = 32;
inline constexpr s32 MAX_UNIFORM_BLOCK_FIELDS  = 16;

inline constexpr u32 MAX_UNIFORM_VALUE_CACHE_SIZE = KB(16);

enum Uniform_Type {
	UNIFORM_U32,
	UNIFORM_F32,
	UNIFORM_F32_2,
	UNIFORM_F32_3,
	UNIFORM_F32_4,
	UNIFORM_F32_4X4,
};

struct Uniform_Light {
    vec3 location;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Uniform_Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    f32  shininess;
};

struct Uniform {
	const char *name = null;
	Uniform_Type type;
	s32 count = 1;
};

struct Uniform_Value_Cache {
    void *data   = null;
    u32 size     = 0;
    u32 capacity = 0;
};

struct Uniform_Buffer {
    u32 id = 0;
    u32 size = 0;
    s32 block_indices[MAX_UNIFORM_BUFFER_BLOCKS];
    s32 block_count = 0;
};

struct Uniform_Block_Field {
    Uniform_Type type;
	s32 count = 1;
};

struct Uniform_Block {
	const char *name = null;
    Uniform_Block_Field fields[MAX_UNIFORM_BLOCK_FIELDS];
    s32 field_count = 0;
    s32 shader_binding = INVALID_INDEX; // must be unique for each block
    u32 offset   = 0; // gpu aligned offset in uniform buffer
    u32 cpu_size = 0; // cpu aligned size
    u32 gpu_size = 0; // gpu aligned size
    s32 uniform_buffer_index = INVALID_INDEX;
};

s32  create_uniform(const char *name, Uniform_Type type, s32 element_count);
u32  cache_uniform_value_on_cpu(s32 uniform_index, const void *data);
void update_uniform_value_on_cpu(s32 uniform_index, const void *data, u32 offset);
void send_uniform_value_to_gpu(s32 shader_index, s32 uniform_index, u32 offset);
u32  get_uniform_type_size(Uniform_Type type);
u32  get_uniform_type_alignment(Uniform_Type type);
u32  get_uniform_type_size_gpu_aligned(Uniform_Type type);

s32 create_uniform_buffer(u32 size);
u32 get_uniform_buffer_used_size(u32 ubi);
u32 get_uniform_buffer_used_size_gpu_aligned(u32 ubi);
s32 create_uniform_block(s32 uniform_buffer_index, s32 shader_binding, const char *name, const Uniform_Block_Field *fields, s32 field_count);
u32 get_uniform_block_field_size(const Uniform_Block_Field &field);
u32 get_uniform_block_field_size_gpu_aligned(const Uniform_Block_Field &field);
u32 get_uniform_block_field_offset_gpu_aligned(s32 uniform_block_index, s32 field_index, s32 field_element_index);
u32 get_uniform_block_size_gpu_aligned(s32 uniform_block_index);
void set_uniform_block_value(s32 ubbi, u32 offset, const void *data, u32 size);
void set_uniform_block_value(s32 ubbi, s32 field_index, s32 field_element_index, const void *data, u32 size);
