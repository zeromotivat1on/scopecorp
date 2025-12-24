#pragma once

void gpu_memory_barrier(); // @Cleanup

union Gpu_Handle {
    u32 _u32;
    u64 _u64;
    void *_p;
};

inline constexpr auto GPU_HANDLE_NONE = Gpu_Handle {0};
    
inline bool operator== (Gpu_Handle a, Gpu_Handle b) { return a._u64 == b._u64; }
inline bool operator!= (Gpu_Handle a, Gpu_Handle b) { return !(a == b); }

inline bool is_valid (Gpu_Handle a) { return a != GPU_HANDLE_NONE; }

enum Gpu_Resource_Type : u8 {
    GPU_RESOURCE_TYPE_NONE,
    GPU_TEXTURE_2D,
    GPU_TEXTURE_2D_ARRAY,
    GPU_CONSTANT_BUFFER,
    GPU_MUTABLE_BUFFER,
};

struct Gpu_Resource {
    Gpu_Resource_Type type = {};
    u16 binding = 0;
    String name;
};

Gpu_Resource make_gpu_resource (String name, Gpu_Resource_Type type, u16 binding);

enum Gpu_Buffer_Bits : u32 {
    GPU_DYNAMIC_BIT    = 0x1,  // allow direct buffer update by client
    GPU_READ_BIT       = 0x2,  // buffer data can be mapped for read access
    GPU_WRITE_BIT      = 0x4,  // buffer data can be mapped for write access
    GPU_PERSISTENT_BIT = 0x8,  // mapped pointer remains valid as long as buffer is mapped
    GPU_COHERENT_BIT   = 0x10, // buffer data sync is implicit and done by backend
};

struct Gpu_Buffer {
    Gpu_Handle handle = GPU_HANDLE_NONE;
    u32 bits = 0;
    u64 size = 0;
    u64 used = 0;
    
    void *mapped_pointer = null;
};

Gpu_Buffer make_gpu_buffer (u64 size, u32 bits);

void  map   (Gpu_Buffer *buffer);
void  map   (Gpu_Buffer *buffer, u64 size, u32 bits);
bool  unmap (Gpu_Buffer *buffer);
void *alloc (Gpu_Buffer *buffer, u64 size);

void *gpu_allocator_proc (Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data);

// Main buffer for all gpu allocations.
inline Gpu_Buffer *gpu_main_buffer = null;
// Main gpu allocator, uses main gpu buffer.
inline Allocator gpu_allocator;

Gpu_Buffer *get_gpu_buffer        ();
Gpu_Handle  get_gpu_buffer_handle ();
u64         get_gpu_buffer_mark   ();
u64         get_gpu_buffer_offset (const void *p);
void       *get_gpu_buffer_data   (u64 offset, u64 size); // size is to ensure data position in buffer is correct

#define __get_gpu_new_macro(_1, _2, name, ...) name
#define __gpu_new1(T)       (T *)alloc((1) * sizeof(T), gpu_allocator)
#define __gpu_new2(T, N)    (T *)alloc((N) * sizeof(T), gpu_allocator)
#define Gpu_New(...) __expand(__get_gpu_new_macro(__VA_ARGS__, __gpu_new2, __gpu_new1)(__VA_ARGS__))

enum Gpu_Sync_Result : u8 {
    GPU_ALREADY_SIGNALED,
    GPU_TIMEOUT_EXPIRED,
    GPU_SIGNALED,
    GPU_WAIT_FAILED,
};

extern const u64 GPU_WAIT_INFINITE;

Gpu_Handle      gpu_fence_sync   ();
Gpu_Sync_Result wait_client_sync (Gpu_Handle sync, u64 timeout);
void            wait_gpu_sync    (Gpu_Handle sync);
void            delete_gpu_sync  (Gpu_Handle sync);
