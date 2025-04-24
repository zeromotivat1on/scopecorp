#pragma once

#include "hash_table.h"

#define SID(s) cache_sid(s)

inline constexpr s32 MAX_SID_TABLE_SIZE = 512;

typedef u64 sid;
typedef Hash_Table<sid, const char *> Sid_Table;

inline Sid_Table sid_table;

void init_sid_table();
sid  cache_sid(const char *string);
