#include "sid.h"
#include "hash.h"
#include "str.h"

constexpr u16 MAX_SID_TABLE_SIZE  = 512;
constexpr u32 MAX_SID_BUFFER_SIZE = KB(16);

typedef Hash_Table<sid, const char *> Sid_Table;

static Sid_Table sid_table = {};
static char *sid_buffer = null; // storage for interned strings
static u32 sid_buffer_size = 0;

static u64 sid_table_hash(const sid &a) {
    return a;
}

void sid_init() {
    sid_table = Sid_Table(MAX_SID_TABLE_SIZE);
    sid_table.hash_function = &sid_table_hash;

    sid_buffer = (char *)allocp(MAX_SID_BUFFER_SIZE);
}

sid sid_intern(const char *string) {
    const sid hash = (sid)hash_fnv(string);
    if (find(sid_table, hash) == null) {        
        const u32 size = (u32)str_size(string) + 1;
        Assert(size + sid_buffer_size < MAX_SID_BUFFER_SIZE);

        char *destination = sid_buffer + sid_buffer_size;
        mem_copy(destination, string, size);
        add(sid_table, hash, (const char *)destination);
        
        sid_buffer_size += size;
    }
    return hash;
}

const char *sid_str(sid sid) {
    const char *str = null;
    if (const auto **v = find(sid_table, sid)) {
        str = *v;
    }
    return str;
}
