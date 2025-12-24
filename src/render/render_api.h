#pragma once

enum class Render_Resource_Type : u32 {
    NONE,

    TEXTURE,                 // any type of texture (2D/3D/Cube/Array etc.)
    RENDER_TARGET,           // writable from gpu texture
    DEPENDENT_RENDER_TARGET, // @Todo?
    BACK_BUFFER,             // screen render target
    CONSTANT_BUFFER,         // shader level buffer with explicit update
    VERTEX_STREAM,           // vertex buffer
    INDEX_STREAM,            // index buffer
    RAW_BUFFER,              // raw gpu buffer, can be writable by gpu
    BATCH_INFO,              // draw call info
    VERTEX_DECLARATION,      // description of vertex stream(s)
    SHADER,                  // pipeline state object (PSO)

    COUNT
};

// Representation of gpu resource.
struct Render_Resource {
    using enum Render_Resource_Type;
    u32 handle;
};

inline auto get_type(const Render_Resource &resource) {
    return (Render_Resource_Type)(resource.handle << 24);
}

inline auto get_index(const Render_Resource &resource) {
    return (resource.handle & 0x00FFFFFF);
}

inline auto set_type(Render_Resource &resource, Render_Resource_Type type) {
    Assert((u32)type < (u32)Render_Resource_Type::COUNT);
    resource.handle = (resource.handle & 0x00FFFFFF) | ((u32)type << 24);
}

inline auto set_index(Render_Resource &resource, u32 index) {
    Assert(index < (1u << 24));
    resource.handle = (resource.handle & 0xFF000000) | (index);
}

// System for render resource creation/destruction and transfer to render backend.
struct Render_Resource_Context {
    
};

struct Render_Command {
    u64 sort_key = 0;
    void *data = null;
    u32 bits = 0;
};

// System for render command collection and transfer to render backend.
struct Render_Context {
    
};
