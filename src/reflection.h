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
    
    FIELD_VECTOR2,
    FIELD_VECTOR3,
    FIELD_VECTOR4,
    FIELD_MATRIX2,
    FIELD_MATRIX3,
    FIELD_MATRIX4,
    FIELD_QUATERNION,
    
    //FIELD_STRUCT,
};

struct Reflect_Field {
    String name;
    u32 offset = 0;
    Reflect_Field_Type type = FIELD_NONE;
};

#define REFLECT_BEGIN(t) inline const Reflect_Field t##_fields[] = {
#define REFLECT_FIELD(t, fn, ft) { S(#fn), offset_of(t, fn), ft },
#define REFLECT_END(t) }; inline constexpr u32 t##_field_count = carray_count(t##_fields);

#define REFLECT_FIELD_COUNT(t) t##_field_count
#define REFLECT_FIELD_AT(t, i) t##_fields[i]

template <typename R, typename T>
R &reflect_field_cast(const T &object, const Reflect_Field &field) {
    return *(R *)((u8 *)&object + field.offset);
}
