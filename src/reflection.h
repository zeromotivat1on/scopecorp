#pragma once

enum Reflect_Field_Type : u8 {
    FIELD_NONE,
    
    FIELD_S8,
    FIELD_S16,
    FIELD_S32,
    FIELD_S64,
    FIELD_U8,
    FIELD_U16,
    FIELD_U32,
    FIELD_U64,
    FIELD_F32,
    FIELD_F64,
    FIELD_BOOL,
    FIELD_CHAR,

    FIELD_SID,
    
    FIELD_VEC2,
    FIELD_VEC3,
    FIELD_VEC4,
    FIELD_MAT2,
    FIELD_MAT3,
    FIELD_MAT4,
    FIELD_QUAT,
    
    //FIELD_STRUCT,
};

struct Reflect_Field {
    const char *name = null;
    u32 offset = 0;
    Reflect_Field_Type type = FIELD_NONE;
};

#define REFLECT_BEGIN(t) inline const Reflect_Field t##_fields[] = {
#define REFLECT_FIELD(t, fn, ft) { #fn, Offsetof(t, fn), ft },
#define REFLECT_END(t) }; inline constexpr u32 t##_field_count = COUNT(t##_fields);

#define REFLECT_FIELD_COUNT(t) t##_field_count
#define REFLECT_FIELD_AT(t, i) t##_fields[i]

template <typename R, typename T>
R &reflect_field_cast(const T &object, const Reflect_Field &field) {
    return *(R *)((u8 *)&object + field.offset);
}
