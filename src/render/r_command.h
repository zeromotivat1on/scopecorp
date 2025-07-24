#pragma once

#define R_CMD_INDEXED_BIT 0x1

#define R_GAME_LAYER 0
#define R_HUD_LAYER  1

#define R_OPAQUE           0
#define R_NORM_TRANSLUCENT 1
#define R_ADD_TRANSLUCENT  2
#define R_SUB_TRANSLUCENT  3

union R_Sort_Key {
    u64 _u64 = 0;
    
    // @Cleanup: we assume we are running on little-endian, handle endianness?
    // Or just avoid bitfields.
    struct {
        u64 padd         : 12; // 64

        u64 material     : 16; // 52
        u64 depth        : 24; // 36
        u64 translucency : 2;  // 12
        u64 target_layer : 2;  // 10
        u64 target       : 6;  // 8
        u64 screen_layer : 2;  // 2
    };
};

static_assert(sizeof(R_Sort_Key) == sizeof(u64));

struct R_Command {    
    u32 bits = 0;
    u16 mode = 0;

    // @Note: if this command will be able to change render target/pass,
    // how to handle sorting then?
    //u16 target = 0;
    //u16 pass   = 0;
    
    u16 shader  = 0;
    u16 texture = 0; // @Todo: take multiple textures instead of only one
    
    u16 vertex_desc = 0;
    u16 index_desc  = 0;
    
    u32 first = 0;
    u32 count = 0;

	u32 base_instance  = 0;
	u32 instance_count = 1;

    u16 uniform_count = 0;
    const u16 *uniforms = null;
    
#if DEVELOPER
    u32 eid_offset = 0;
#endif
};

struct R_Command_List {
    struct Queued {
        R_Sort_Key sort_key;
        R_Command command;
    };
    
    u32 count = 0;
    u32 capacity = 0;
    Queued *queued = null;

    rid command_buffer = RID_NONE;
    rid command_buffer_indexed = RID_NONE;
};

// @Cleanup: this command list is used/reused for different purposes;
// maybe its not bad but it could lead to possible issues, make sure
// its submitted with correct queued commands.
inline R_Command_List R_command_list;

void r_add(R_Command_List &list, const R_Command &cmd, R_Sort_Key sort_key = { 0 });

void r_create_command_list(u32 capacity, R_Command_List &list);
void r_submit(R_Command_List &list);
