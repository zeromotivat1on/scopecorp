#pragma once

#include "math/vector.h"

// Basic linal matrices (2, 3, 4).
// Matrices follow *row-major* convention.
// Matrix operations follow *right-handed* convention.

struct quat;

inline constexpr f32 MATRIX_EPSILON	    = 1e-06f;
inline constexpr f32 MATRIX_INV_EPSILON	= 1e-14f;

struct mat2 {
    union {
        vec2 v[2];
        f32  e[4];
    };

    mat2()                             { mem_set(e, 0, 4 * sizeof(f32)); }
    mat2(const vec2 &a, const vec2 &b) { v[0] = a; v[1] = b; }
    mat2(const f32 a[4])               { mem_copy(e, a, 4 * sizeof(f32)); }
    mat2(const f32 a[2][2])            { mem_copy(e, a, 4 * sizeof(f32)); }
    mat2(f32 a, f32 b, f32 c, f32 d)   { e[0] = a; e[1] = b; e[2] = c; e[3] = d; }
    
    inline vec2       &operator[](s32 index)       { return v[index]; }
    inline const vec2 &operator[](s32 index) const { return v[index]; }
};

struct mat3 {
    union {
        vec3 v[3];
        f32  e[9];
    };

    mat3()                                            { mem_set(e, 0, 9 * sizeof(f32)); }
    mat3(const vec3 &a, const vec3 &b, const vec3 &c) { v[0] = a; v[1] = b; v[2] = c; }
    mat3(const f32 a[9])                              { mem_copy(e, a, 9 * sizeof(f32)); }
    mat3(const f32 a[3][3])                           { mem_copy(e, a, 9 * sizeof(f32)); }
    mat3(const quat &a);
    mat3(f32 a, f32  b, f32 c, f32 d, f32 _e, f32 f, f32 g, f32  h, f32 i) {
        e[0] = a; e[1] =  b; e[2] = c;
        e[3] = d; e[4] = _e; e[5] = f;
        e[6] = g; e[7] =  h; e[8] = i;
    }
    
    inline vec3       &operator[](s32 index)       { return v[index]; }
    inline const vec3 &operator[](s32 index) const { return v[index]; }
};

struct mat4 {
    union {
        vec4 v[4];
        f32  e[16];
    };
        
    mat4() { mem_set(e, 0, 16 * sizeof(f32)); }
    mat4(const vec4 &a, const vec4 &b,
         const vec4 &c, const vec4 &d) { v[0] = a; v[1] = b; v[2] = c; v[3] = d; }
    mat4(const f32 a[16])   { mem_copy(e, a, 16 * sizeof(f32)); }
    mat4(const f32 a[4][4]) { mem_copy(e, a, 16 * sizeof(f32)); }
    mat4(const mat3 &a) {
        v[0] = {a[0], 0}; v[1] = {a[1], 0}; v[2] = {a[2], 0}; v[3] = {0, 0, 0, 1};
    }
    mat4(const mat3& a, const vec3& b) {
        v[0] = {a[0], b[0]}; v[1] = {a[1], b[1]}; v[2] = {a[2], b[2]}; v[3] = {0, 0, 0, 1};
    }
    mat4(const quat &a) {
        *this = mat4(mat3(a));
    }
    mat4(f32  a, f32 b, f32 c, f32 d, f32 _e, f32 f, f32 g, f32 h,
         f32  i, f32 j, f32 k, f32 l, f32  m, f32 n, f32 o, f32 p) {
        e[0]  =  a; e[1]  = b; e[2]  = c; e[3]  = d;
        e[4]  = _e; e[5]  = f; e[6]  = g; e[7]  = h;
        e[8]  =  i; e[9]  = j; e[10] = k; e[11] = l;
        e[12] =  m; e[13] = n; e[14] = o; e[15] = p;
    }

    inline vec4       &operator[](s32 index)       { return v[index]; }
    inline const vec4 &operator[](s32 index) const { return v[index]; }
};

mat2 mat2_identity();
mat3 mat3_identity();
mat4 mat4_identity();

mat2 operator-(const mat2 &a);
mat3 operator-(const mat3 &a);
mat4 operator-(const mat4 &a);

mat2 operator+(const mat2 &a, const mat2 &b);
mat3 operator+(const mat3 &a, const mat3 &b);
mat4 operator+(const mat4 &a, const mat4 &b);

mat2 operator-(const mat2 &a, const mat2 &b);
mat3 operator-(const mat3 &a, const mat3 &b);
mat4 operator-(const mat4 &a, const mat4 &b);

mat2 operator*(const mat2 &a, const mat2 &b);
mat3 operator*(const mat3 &a, const mat3 &b);
mat4 operator*(const mat4 &a, const mat4 &b);

mat2 operator*(const mat2 &a, f32 b);
mat3 operator*(const mat3 &a, f32 b);
mat4 operator*(const mat4 &a, f32 b);

vec2 operator*(const mat2 &a, const vec2 &b);
vec3 operator*(const mat3 &a, const vec3 &b);
vec4 operator*(const mat4 &a, const vec4 &b);

mat2 &operator+=(mat2 &a, const mat2 &b);
mat3 &operator+=(mat3 &a, const mat3 &b);
mat4 &operator+=(mat4 &a, const mat4 &b);

mat2 &operator-=(mat2 &a, const mat2 &b);
mat3 &operator-=(mat3 &a, const mat3 &b);
mat4 &operator-=(mat4 &a, const mat4 &b);

mat2 &operator*=(mat2 &a, const mat2 &b);
mat3 &operator*=(mat3 &a, const mat3 &b);
mat4 &operator*=(mat4 &a, const mat4 &b);

mat2 &operator*=(mat2 &a, f32 b);
mat3 &operator*=(mat3 &a, f32 b);
mat4 &operator*=(mat4 &a, f32 b);

bool operator==(const mat2 &a, const mat2 &b);
bool operator==(const mat3 &a, const mat3 &b);
bool operator==(const mat4 &a, const mat4 &b);

bool operator!=(const mat2 &a, const mat2 &b);
bool operator!=(const mat3 &a, const mat3 &b);
bool operator!=(const mat4 &a, const mat4 &b);

mat2 operator*(f32 a, const mat2 &b);
mat3 operator*(f32 a, const mat3 &b);
mat4 operator*(f32 a, const mat4 &b);

vec2 operator*(const vec2 &a, const mat2 &b);
vec3 operator*(const vec3 &a, const mat3 &b);
vec3 operator*(const vec3 &a, const mat4 &b);
vec4 operator*(const vec4 &a, const mat4 &b);

vec2 &operator*=(vec2 &a, const mat2 &b);
vec3 &operator*=(vec3 &a, const mat3 &b);
vec3 &operator*=(vec3 &a, const mat4 &b);
vec4 &operator*=(vec4 &a, const mat4 &b);

bool equal(const mat2 &a, const mat2 &b, f32 c = F32_EPSILON);
bool equal(const mat3 &a, const mat3 &b, f32 c = F32_EPSILON);
bool equal(const mat4 &a, const mat4 &b, f32 c = F32_EPSILON);

bool identity(const mat2 &a, f32 b = MATRIX_EPSILON);
bool identity(const mat3 &a, f32 b = MATRIX_EPSILON);
bool identity(const mat4 &a, f32 b = MATRIX_EPSILON);

bool symmetric(const mat2 &a, f32 b = MATRIX_EPSILON);
bool symmetric(const mat3 &a, f32 b = MATRIX_EPSILON);
bool symmetric(const mat4 &a, f32 b = MATRIX_EPSILON);

bool diagonal(const mat2 &a, f32 b = MATRIX_EPSILON);
bool diagonal(const mat3 &a, f32 b = MATRIX_EPSILON);
bool diagonal(const mat4 &a, f32 b = MATRIX_EPSILON);

bool rotated(const mat3 &a);
bool rotated(const mat4 &a);

mat2 transpose(const mat2 &a);
mat3 transpose(const mat3 &a);
mat4 transpose(const mat4 &a);

f32 trace(const mat2 &a);
f32 trace(const mat3 &a);
f32 trace(const mat4 &a);

mat4 translate(const mat4 &a, const vec3 &b);
mat4 rotate(const mat4 &a, const mat4 &b);
mat4 rotate(const mat4 &a, const quat &b);
mat4 scale(const mat4 &a, const vec3 &b);

mat4 inverse(const mat4 &m);

mat4 mat4_transform(const vec3& location, const quat& rotation, const vec3& scale);
mat4 mat4_view(const vec3& eye, const vec3& at, const vec3& up);
mat4 mat4_perspective(f32 rfovy, f32 aspect, f32 n, f32 f);
mat4 mat4_orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
