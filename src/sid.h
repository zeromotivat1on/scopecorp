#pragma once

#include "hash_table.h"

// @Speed: this will intern sid on every call which may be not so optimal.
#define SID(s) sid_intern(s)

void sid_init();
sid  sid_intern(const char *string);
const char *sid_str(sid sid);
