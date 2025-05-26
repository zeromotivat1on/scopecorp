#pragma once

inline constexpr s32 MAX_SHADER_SIZE = KB(64);

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
    sid ui_element;
};

inline Shader_Sid_List shader_sids;

void cache_shader_sids(Shader_Sid_List *list);

s32  create_shader(const char *source, const char *path); // path to log during errors
bool recreate_shader(s32 shader_index, const char *source, const char *path);
bool parse_shader(const char *source, char *out_vertex, char *out_fragment);

const char *get_desired_shader_file_extension();
