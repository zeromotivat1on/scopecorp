#pragma once

enum Reflection_Field_Type : u8 {
    REFLECTION_FIELD_TYPE_NONE,
    REFLECTION_FIELD_TYPE_S8,
    REFLECTION_FIELD_TYPE_S16,
    REFLECTION_FIELD_TYPE_S32,
    REFLECTION_FIELD_TYPE_S64,
    REFLECTION_FIELD_TYPE_U8,
    REFLECTION_FIELD_TYPE_U16,
    REFLECTION_FIELD_TYPE_U32,
    REFLECTION_FIELD_TYPE_U64,
    REFLECTION_FIELD_TYPE_F32,
    REFLECTION_FIELD_TYPE_F64,
    REFLECTION_FIELD_TYPE_BOOL,
    REFLECTION_FIELD_TYPE_CHAR,
    REFLECTION_FIELD_TYPE_VECTOR2,
    REFLECTION_FIELD_TYPE_VECTOR3,
    REFLECTION_FIELD_TYPE_VECTOR4,
    REFLECTION_FIELD_TYPE_MATRIX2,
    REFLECTION_FIELD_TYPE_MATRIX3,
    REFLECTION_FIELD_TYPE_MATRIX4,
    REFLECTION_FIELD_TYPE_QUATERNION,
    REFLECTION_FIELD_TYPE_STRING,
    REFLECTION_FIELD_TYPE_ATOM,
    REFLECTION_FIELD_TYPE_PID,
    REFLECTION_FIELD_TYPE_AABB,
};

struct Reflection_Field {
    Reflection_Field_Type type;
    String                name;
    u32                   offset;
};

#define Begin_Reflection(t) inline const Reflection_Field t##_fields[] = {
#define Add_Reflection_Field(t, fn, ft) { ft, S(#fn), offset_of(t, fn), },
#define End_Reflection(t) };
#define Reflection_Field_Count(t) carray_count(t##_fields)
#define Reflection_Field_At(t, i) &t##_fields[i]

inline const String reflection_field_type_names[] = {
    S(""),
    S("s8"),
    S("s16"),
    S("s32"),
    S("s64"),
    S("u8"),
    S("u16"),
    S("u32"),
    S("u64"),
    S("f32"),
    S("f64"),
    S("bool"),
    S("char"),
    S("Vector2"),
    S("Vector3"),
    S("Vector4"),
    S("Matrix2"),
    S("Matrix3"),
    S("Matrix4"),
    S("Quaternion"),
    S("String"),
    S("Atom"),
    S("Pid"),
    S("AABB"),
};

inline String to_string(Reflection_Field_Type type) {
    return reflection_field_type_names[type];
}

inline Reflection_Field_Type get_reflection_type(String name) {
    for (u32 i = 1; i < carray_count(reflection_field_type_names); ++i) {
        const auto type_name = reflection_field_type_names[i];
        if (name == type_name) return (Reflection_Field_Type)i;
    }

    log(LOG_ERROR, "Failed to get reflection type from %S", name);
    return REFLECTION_FIELD_TYPE_NONE;
}
