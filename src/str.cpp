#include "pch.h"
#include "str.h"
#include <string.h>

void str_copy(char *dst, const char *src) {
    strcpy(dst, src);
}

bool str_compare(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

char *str_char(char *str, s32 c) {
    return strchr(str, c);
}

char *str_char_from_end(char *str, s32 c) {
    return strrchr(str, c);
}
