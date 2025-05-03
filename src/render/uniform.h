#pragma once

#include "math/vector.h"

inline constexpr u32 MAX_UNIFORM_VALUE_CACHE_SIZE = KB(16);

enum Uniform_Type {
	UNIFORM_U32,
	UNIFORM_F32,
	UNIFORM_F32_2,
	UNIFORM_F32_3,
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
	s32 count =  1;
};

struct Uniform_Value_Cache {
    void *data   = null;
    u32 size     = 0;
    u32 capacity = 0;
};

s32  create_uniform(const char *name, Uniform_Type type, s32 element_count);
u32  cache_uniform_value_on_cpu(s32 uniform_index, const void *data);
void update_uniform_value_on_cpu(s32 uniform_index, const void *data, u32 offset);
void send_uniform_value_to_gpu(s32 shader_index, s32 uniform_index, u32 offset);
u32  uniform_value_type_size(Uniform_Type type);
