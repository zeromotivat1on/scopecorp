#pragma once

struct Ring_Buffer {
    void *data;
    u64   size;
    u64   read;
    u64   written;
};

inline void init(Ring_Buffer *rb, u64 size, Allocator alc) {
    rb->data = alloc(size, alc);
    rb->size = size;
    rb->read = 0;
    rb->written = 0;
}

inline Ring_Buffer make_ring_buffer(u64 size, Allocator alc) {
    Ring_Buffer rb;
    init(&rb, size, alc);
    return rb;
}

inline void write(Ring_Buffer *rb, void *p, u64 n) {
    auto offset = rb->written % rb->size;
    auto space = rb->size - offset;
    auto first_write = n < space ? n : space;

    copy((u8 *)rb->data + offset, p, first_write);
    copy(rb->data, (u8 *)p + first_write, n - first_write);

    rb->written += n;
}

inline void *read(Ring_Buffer *rb, u64 n) {
    
}
