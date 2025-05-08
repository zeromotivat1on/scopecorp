#pragma once

u64  str_size(const char *str);
void str_copy(char *dst, const char *src);
void str_glue(char *dst, const char *src);
bool str_cmp(const char *a, const char *b);
char *str_sub(char *str, const char *sub);
char *str_char(char *str, s32 c);
char *str_char_from_end(char *str, s32 c);
const char *str_sub(const char *str, const char *sub);
const char *str_char(const char *str, s32 c);
const char *str_char_from_end(const char *str, s32 c);
