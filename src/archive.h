#pragma once

#include "file_system.h"

enum Archive_Mode : u32 {
    ARCHIVE_MODE_NONE,
    ARCHIVE_MODE_READ,
    ARCHIVE_MODE_WRITE,
};

// Simple serialization wrapper.
struct Archive {
    File         file;
    Archive_Mode mode;
};

bool start_read       (Archive &archive, String path);
bool start_write      (Archive &archive, String path, bool truncate);
bool set_pointer      (Archive &archive, u64 position);
u64  get_current_size (Archive &archive);
u64  serialize        (Archive &archive, void *data, u64 size);
void reset            (Archive &archive);

template <typename T>
u64 serialize(Archive &archive, T &data) {
    return serialize(archive, &data, sizeof(data));
}
