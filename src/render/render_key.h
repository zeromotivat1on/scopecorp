#pragma once

// Main render key bits.
inline constexpr u64 RENDER_KEY_SCREEN_LAYER_BITS   = 2;
inline constexpr u64 RENDER_KEY_VIEWPORT_BITS       = 3;
inline constexpr u64 RENDER_KEY_VIEWPORT_LAYER_BITS = 3;
inline constexpr u64 RENDER_KEY_TRANSLUCENY_BITS    = 2;
inline constexpr u64 RENDER_KEY_IS_COMMAND_BIT      = 1;
// Depending on is_command bit, next bits are either for command or direct draw call.

// Command bits.
inline constexpr u64 RENDER_KEY_SEQUENCE_BITS       = 21;
inline constexpr u64 RENDER_KEY_COMMAND_BITS        = 32;

// Draw call bits.
inline constexpr u64 RENDER_KEY_DEPTH_BITS          = 21;
inline constexpr u64 RENDER_KEY_MATERIAL_BITS       = 32;

enum Screen_Layer_Type : u8 {
    SCREEN_GAME_LAYER,
    SCREEN_VFX_LAYER,
    SCREEN_HUD_LAYER,
};

enum Viewport_Layer_Type : u8 {
    VIEWPORT_GAME_LAYER,
    VIEWPORT_VFX_LAYER,
    VIEWPORT_HUD_LAYER,
};

enum Translucency_Type : u8 {
    NOT_TRANSLUCENT,
    NORM_TRANSLUCENT,
    ADD_TRANSLUCENT,
    SUB_TRANSLUCENT,
};

// Generate render sort key for sorting before submitting to backend.
// Note that depth and material bits are swapped depending on translucency,
// as depth is more relevant for transparents and material for opaques.
inline u64 make_render_key(Screen_Layer_Type screen_layer, u8 viewport,
                           Viewport_Layer_Type viewport_layer,
                           Translucency_Type translucency,
                           bool is_command, u64 y, u64 z) {
    Assert(viewport < (1u << RENDER_KEY_VIEWPORT_BITS));
    
    u64 key = 0;
    s64 shift = 64;

    const auto push = [&](u64 value, u64 bits) {
        shift -= bits; Assert(shift >= 0);
        key |= (value & ((1ull << bits) - 1)) << shift;
    };

    push(screen_layer,   RENDER_KEY_SCREEN_LAYER_BITS);
    push(viewport,       RENDER_KEY_VIEWPORT_BITS);
    push(viewport_layer, RENDER_KEY_VIEWPORT_LAYER_BITS);
    push(translucency,   RENDER_KEY_TRANSLUCENY_BITS);
    push(is_command,     RENDER_KEY_IS_COMMAND_BIT);

    if (is_command) {
        const auto sequence = y; Assert(sequence < (1ull << RENDER_KEY_SEQUENCE_BITS));
        const auto command  = z; Assert(command  < (1ull << RENDER_KEY_COMMAND_BITS));

        push(sequence, RENDER_KEY_SEQUENCE_BITS);
        push(command,  RENDER_KEY_COMMAND_BITS);
    } else {
        const auto depth    = y; Assert(depth    < (1ull << RENDER_KEY_DEPTH_BITS));
        const auto material = z; Assert(material < (1ull << RENDER_KEY_MATERIAL_BITS));
        
        const bool transparent = (translucency != NOT_TRANSLUCENT);
        if (transparent) {
            // Depth dominates - correct blending.
            push(depth,    RENDER_KEY_DEPTH_BITS);
            push(material, RENDER_KEY_MATERIAL_BITS);
        } else {
            // Material dominates - state batching.
            push(material, RENDER_KEY_MATERIAL_BITS);
            push(depth,    RENDER_KEY_DEPTH_BITS);
        }
    }
    
    return key;    
}

union Render_Key {
    u64 _u64 = 0;
    
    struct {
        u64 material       : 32;
        u64 depth          : 21;
        u64 is_command     : 1;
        u64 translucency   : 2; 
        u64 viewport_layer : 3; 
        u64 viewport       : 3; 
        u64 screen_layer   : 2; 
    };
};

static_assert(sizeof(Render_Key) == sizeof(u64));
