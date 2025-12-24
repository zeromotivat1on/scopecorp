#pragma once

//
// Simple buffer with linear allocation and hard wrap to start in case of overflow.
// Generally, overflow wrap is to ensure alloc always returns a valid pointer, but
// its suggested to clear the buffer after use. Buffer spams annoying overflow logs.
//
struct Wrap_Buffer {
    void *data;
    u64   size;
    u64   used;
};

inline Wrap_Buffer make_wrap_buffer(u64 size, Allocator alc) {
    Wrap_Buffer buffer = {};
    buffer.data = alloc(size, alc);
    buffer.size = size;
    return buffer;
}

inline void reset(Wrap_Buffer *buffer) {
    buffer->used = 0;
}

inline void align(Wrap_Buffer *buffer, u64 alignment) {
    buffer->used = Align(buffer->used, alignment);
    if (buffer->used > buffer->size) {
        log(LOG_MINIMAL, "Wrap buffer 0x%X overflow after align", buffer);
        buffer->used = 0;
    }
}

inline void *alloc(Wrap_Buffer *buffer, u64 size) {
    auto &data = buffer->data;
    auto &used = buffer->used;

    if (used + size > buffer->size) {
        log(LOG_MINIMAL, "Wrap buffer 0x%X overflow after alloc", buffer);
        used = 0;
    }
        
    auto p = (u8 *)data + used;
    used += size;
    
    return p;
}
