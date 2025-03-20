#pragma once

enum Uniform_Type {
	UNIFORM_NULL,
	UNIFORM_U32,
	UNIFORM_F32,
	UNIFORM_F32_2,
	UNIFORM_F32_3,
	UNIFORM_F32_4X4,
};

enum Uniform_Flags : u32 {
	UNIFORM_FLAG_DIRTY = 0x1,
};

struct Uniform {
	Uniform() = default;
	Uniform(const char *name, Uniform_Type type, s32 count)
		: name(name), type(type), count(count) {}

	const char *name  = null;
	const void *value = null;
	Uniform_Type type = UNIFORM_NULL;
	u32 flags    =  0;
	s32 location = -1;
	s32 count    =  0; // greater than 1 for array uniforms
};

void sync_uniform(const Uniform *uniform); // pass uniform data to gpu if dirty
