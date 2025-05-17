#include "pch.h"
#include "str.h"
#include <string.h>

u64 str_size(const char *str) {
    return strlen(str);
}

void str_copy(char *dst, const char *src) {
    strcpy(dst, src);
}

void str_copy(char *dst, const char *src, u64 n) {
    strncpy(dst, src, n);
}

void str_glue(char *dst, const char *src) {
    strcat(dst, src);
}

void str_glue(char *dst, const char *src, u64 n) {
    strncat(dst, src, n);
}

bool str_cmp(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

bool str_cmp(const char *a, const char *b, u64 n) {
    return strncmp(a, b, n) == 0;
}

char *str_sub(char *str, const char *sub) {
    return strstr(str, sub);
}

char *str_char(char *str, s32 c) {
    return strchr(str, c);
}

char *str_char_from_end(char *str, s32 c) {
    return strrchr(str, c);
}

const char *str_sub(const char *str, const char *sub) {
    return strstr(str, sub);
}

const char *str_char(const char *str, s32 c) {
    return strchr(str, c);
}

const char *str_char_from_end(const char *str, s32 c) {
    return strrchr(str, c);
}
