#pragma once

inline constexpr s32 MAX_SHADER_SIZE = KB(8);

typedef u64 sid;

struct Shader {
	u32 id;
};

struct Shader_Sid_List {
	sid entity;
	sid text;
	sid skybox;
	sid frame_buffer;
    sid geometry;
    sid outline;
};

inline Shader_Sid_List shader_sids;

void cache_shader_sids(Shader_Sid_List *list);

s32  create_shader(const char *source);
bool recreate_shader(s32 shader_index, const char *source);

bool parse_shader_source(const char *shader_src, char *vertex_src, char *fragment_src);
