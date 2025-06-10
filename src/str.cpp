#include "pch.h"
#include "str.h"
#include <stdlib.h>
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

char *str_token(char *str, const char *delimiters) {
    return strtok(str, delimiters);
}

char *str_trim(char *str) {
    if (!str) return null;

    char *first = str;
    char *last  = str_last(str);

    while (is_space(*first)) {
        first += 1;
    }

    while (is_space(*last)) {
        last -= 1;
    }

    *(last + 1) = '\0';

    return first;
}

char *str_last(char *str) {
    const auto size = str_size(str);
    return str + size - 1;
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

bool is_space(s32 c) {
    return c == ASCII_SPACE
        || c == ASCII_TAB
        || c == ASCII_NEW_LINE
        || c == ASCII_FORM_FEED
        || c == ASCII_VERTICAL_TAB
        || c == ASCII_CARRIAGE_RETURN;
}

f32 str_to_f32(const char *str) {
    return (f32)atof(str);
}

u32 str_to_u32(const char *str) {
    return (u32)strtoul(str, null, 0);
}
