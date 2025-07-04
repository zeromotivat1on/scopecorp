#pragma once

u64  str_size(const char *str);
void str_copy(char *dst, const char *src);
void str_copy(char *dst, const char *src, u64 n);
void str_glue(char *dst, const char *src);
void str_glue(char *dst, const char *src, u64 n);
bool str_cmp(const char *a, const char *b);
bool str_cmp(const char *a, const char *b, u64 n);
char *str_sub(char *str, const char *sub);
char *str_char(char *str, s32 c);
char *str_char_from_end(char *str, s32 c);
char *str_token(char *str, const char *delimiters);
char *str_trim(char *str);
char *str_last(char *str);
const char *str_sub(const char *str, const char *sub);
const char *str_char(const char *str, s32 c);
const char *str_char_from_end(const char *str, s32 c);

bool is_space(s32 c);

f32 str_to_f32(const char *str);

s8  str_to_s8(const char *str);
s16 str_to_s16(const char *str);
s32 str_to_s32(const char *str);
s64 str_to_s64(const char *str);

u8  str_to_u8(const char *str);
u16 str_to_u16(const char *str); 
u32 str_to_u32(const char *str);
u64 str_to_u64(const char *str);
