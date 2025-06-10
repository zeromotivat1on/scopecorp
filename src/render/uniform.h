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

enum Uniform_Type : u8 {
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
	char name[MAX_UNIFORM_NAME_SIZE];
	Uniform_Type type;
	s32 count = 1;
    u32 value_offset = INVALID_INDEX; // in uniform value cache
};

struct Uniform_Value_Cache {
    void *data   = null;
    u32 size     = 0;
    u32 capacity = 0;
};

struct Uniform_Block_Field {
    Uniform_Type type;
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

inline Uniform_Value_Cache uniform_value_cache;

inline Uniform_Block uniform_block_camera;
inline Uniform_Block uniform_block_viewport;
inline Uniform_Block uniform_block_direct_lights;
inline Uniform_Block uniform_block_point_lights;

void r_set_uniform(rid rid_shader, const Uniform *uniform);

rid r_create_uniform_buffer(u32 size);
void r_add_uniform_block(rid rid_uniform_buffer, s32 shader_binding, const char *name, const Uniform_Block_Field *fields, s32 field_count, Uniform_Block *block);
void r_set_uniform_block_value(Uniform_Block *block, s32 field_index, s32 field_element_index, const void *data, u32 size);
void r_set_uniform_block_value(Uniform_Block *block, u32 offset, const void *data, u32 size);

void cache_uniform_value_on_cpu(Uniform *uniform, const void *data, u32 data_size, u32 data_offset = 0);
u32 get_uniform_type_size(Uniform_Type type);
u32 get_uniform_type_dimension(Uniform_Type type);
u32 get_uniform_type_alignment(Uniform_Type type);
u32 get_uniform_type_size_gpu_aligned(Uniform_Type type);

u32 get_uniform_block_field_size(const Uniform_Block_Field &field);
u32 get_uniform_block_field_size_gpu_aligned(const Uniform_Block_Field &field);
u32 get_uniform_block_field_offset_gpu_aligned(Uniform_Block *block, s32 field_index, s32 field_element_index);
