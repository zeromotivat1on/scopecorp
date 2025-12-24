#pragma once

#include "vector.h"

// Basic matrices (2, 3, 4).
// Matrices follow *row-major* convention.
// Matrix operations follow *right-handed* convention.

struct Quaternion;

inline constexpr f32 MATRIX_EPSILON	    = 1e-06f;
inline constexpr f32 MATRIX_INV_EPSILON	= 1e-14f;

struct Matrix2 {
    union {
        Vector2 v[2];
        f32  e[4];
    };

    Matrix2()                             { set(e, 0, 4 * sizeof(f32)); }
    Matrix2(const Vector2 &a, const Vector2 &b) { v[0] = a; v[1] = b; }
    Matrix2(const f32 a[4])               { copy(e, a, 4 * sizeof(f32)); }
    Matrix2(const f32 a[2][2])            { copy(e, a, 4 * sizeof(f32)); }
    Matrix2(f32 a, f32 b, f32 c, f32 d)   { e[0] = a; e[1] = b; e[2] = c; e[3] = d; }
    
    inline Vector2       &operator[](s32 index)       { return v[index]; }
    inline const Vector2 &operator[](s32 index) const { return v[index]; }
};

struct Matrix3 {
    union {
        Vector3 v[3];
        f32  e[9];
    };

    Matrix3()                                            { set(e, 0, 9 * sizeof(f32)); }
    Matrix3(const Vector3 &a, const Vector3 &b, const Vector3 &c) { v[0] = a; v[1] = b; v[2] = c; }
    Matrix3(const f32 a[9])                              { copy(e, a, 9 * sizeof(f32)); }
    Matrix3(const f32 a[3][3])                           { copy(e, a, 9 * sizeof(f32)); }
    Matrix3(const Quaternion &a);
    Matrix3(f32 a, f32  b, f32 c, f32 d, f32 _e, f32 f, f32 g, f32  h, f32 i) {
        e[0] = a; e[1] =  b; e[2] = c;
        e[3] = d; e[4] = _e; e[5] = f;
        e[6] = g; e[7] =  h; e[8] = i;
    }
    
    inline Vector3       &operator[](s32 index)       { return v[index]; }
    inline const Vector3 &operator[](s32 index) const { return v[index]; }
};

struct Matrix4 {
    union {
        Vector4 v[4];
        f32  e[16];
    };
        
    Matrix4() { set(e, 0, 16 * sizeof(f32)); }
    Matrix4(const Vector4 &a, const Vector4 &b,
         const Vector4 &c, const Vector4 &d) { v[0] = a; v[1] = b; v[2] = c; v[3] = d; }
    Matrix4(const f32 a[16])   { copy(e, a, 16 * sizeof(f32)); }
    Matrix4(const f32 a[4][4]) { copy(e, a, 16 * sizeof(f32)); }
    Matrix4(const Matrix3 &a) {
        v[0] = {a[0], 0}; v[1] = {a[1], 0}; v[2] = {a[2], 0}; v[3] = {0, 0, 0, 1};
    }
    Matrix4(const Matrix3& a, const Vector3& b) {
        v[0] = {a[0], b[0]}; v[1] = {a[1], b[1]}; v[2] = {a[2], b[2]}; v[3] = {0, 0, 0, 1};
    }
    Matrix4(const Quaternion &a) {
        *this = Matrix4(Matrix3(a));
    }
    Matrix4(f32  a, f32 b, f32 c, f32 d, f32 _e, f32 f, f32 g, f32 h,
         f32  i, f32 j, f32 k, f32 l, f32  m, f32 n, f32 o, f32 p) {
        e[0]  =  a; e[1]  = b; e[2]  = c; e[3]  = d;
        e[4]  = _e; e[5]  = f; e[6]  = g; e[7]  = h;
        e[8]  =  i; e[9]  = j; e[10] = k; e[11] = l;
        e[12] =  m; e[13] = n; e[14] = o; e[15] = p;
    }

    inline Vector4       &operator[](s32 index)       { return v[index]; }
    inline const Vector4 &operator[](s32 index) const { return v[index]; }
};

Matrix2 Matrix2_identity ();
Matrix3 Matrix3_identity ();
Matrix4 Matrix4_identity ();

Matrix2 operator- (const Matrix2 &a);
Matrix3 operator- (const Matrix3 &a);
Matrix4 operator- (const Matrix4 &a);

Matrix2 operator+ (const Matrix2 &a, const Matrix2 &b);
Matrix3 operator+ (const Matrix3 &a, const Matrix3 &b);
Matrix4 operator+ (const Matrix4 &a, const Matrix4 &b);

Matrix2 operator- (const Matrix2 &a, const Matrix2 &b);
Matrix3 operator- (const Matrix3 &a, const Matrix3 &b);
Matrix4 operator- (const Matrix4 &a, const Matrix4 &b);

Matrix2 operator* (const Matrix2 &a, const Matrix2 &b);
Matrix3 operator* (const Matrix3 &a, const Matrix3 &b);
Matrix4 operator* (const Matrix4 &a, const Matrix4 &b);

Matrix2 operator* (const Matrix2 &a, f32 b);
Matrix3 operator* (const Matrix3 &a, f32 b);
Matrix4 operator* (const Matrix4 &a, f32 b);

Vector2 operator* (const Matrix2 &a, const Vector2 &b);
Vector3 operator* (const Matrix3 &a, const Vector3 &b);
Vector4 operator* (const Matrix4 &a, const Vector4 &b);

Matrix2 &operator+= (Matrix2 &a, const Matrix2 &b);
Matrix3 &operator+= (Matrix3 &a, const Matrix3 &b);
Matrix4 &operator+= (Matrix4 &a, const Matrix4 &b);

Matrix2 &operator-= (Matrix2 &a, const Matrix2 &b);
Matrix3 &operator-= (Matrix3 &a, const Matrix3 &b);
Matrix4 &operator-= (Matrix4 &a, const Matrix4 &b);

Matrix2 &operator*= (Matrix2 &a, const Matrix2 &b);
Matrix3 &operator*= (Matrix3 &a, const Matrix3 &b);
Matrix4 &operator*= (Matrix4 &a, const Matrix4 &b);

Matrix2 &operator*= (Matrix2 &a, f32 b);
Matrix3 &operator*= (Matrix3 &a, f32 b);
Matrix4 &operator*= (Matrix4 &a, f32 b);

bool operator== (const Matrix2 &a, const Matrix2 &b);
bool operator== (const Matrix3 &a, const Matrix3 &b);
bool operator== (const Matrix4 &a, const Matrix4 &b);

bool operator!= (const Matrix2 &a, const Matrix2 &b);
bool operator!= (const Matrix3 &a, const Matrix3 &b);
bool operator!= (const Matrix4 &a, const Matrix4 &b);

Matrix2 operator* (f32 a, const Matrix2 &b);
Matrix3 operator* (f32 a, const Matrix3 &b);
Matrix4 operator* (f32 a, const Matrix4 &b);

Vector2 operator* (const Vector2 &a, const Matrix2 &b);
Vector3 operator* (const Vector3 &a, const Matrix3 &b);
Vector3 operator* (const Vector3 &a, const Matrix4 &b);
Vector4 operator* (const Vector4 &a, const Matrix4 &b);

Vector2 &operator*= (Vector2 &a, const Matrix2 &b);
Vector3 &operator*= (Vector3 &a, const Matrix3 &b);
Vector3 &operator*= (Vector3 &a, const Matrix4 &b);
Vector4 &operator*= (Vector4 &a, const Matrix4 &b);

bool equal (const Matrix2 &a, const Matrix2 &b, f32 c = F32_EPSILON);
bool equal (const Matrix3 &a, const Matrix3 &b, f32 c = F32_EPSILON);
bool equal (const Matrix4 &a, const Matrix4 &b, f32 c = F32_EPSILON);

bool identity (const Matrix2 &a, f32 b = MATRIX_EPSILON);
bool identity (const Matrix3 &a, f32 b = MATRIX_EPSILON);
bool identity (const Matrix4 &a, f32 b = MATRIX_EPSILON);

bool symmetric (const Matrix2 &a, f32 b = MATRIX_EPSILON);
bool symmetric (const Matrix3 &a, f32 b = MATRIX_EPSILON);
bool symmetric (const Matrix4 &a, f32 b = MATRIX_EPSILON);

bool diagonal (const Matrix2 &a, f32 b = MATRIX_EPSILON);
bool diagonal (const Matrix3 &a, f32 b = MATRIX_EPSILON);
bool diagonal (const Matrix4 &a, f32 b = MATRIX_EPSILON);

bool rotated (const Matrix3 &a);
bool rotated (const Matrix4 &a);

Matrix2 transpose (const Matrix2 &a);
Matrix3 transpose (const Matrix3 &a);
Matrix4 transpose (const Matrix4 &a);

f32 trace (const Matrix2 &a);
f32 trace (const Matrix3 &a);
f32 trace (const Matrix4 &a);

Matrix4 translate (const Matrix4 &a, const Vector3 &b);
Matrix4 rotate    (const Matrix4 &a, const Matrix4 &b);
Matrix4 rotate    (const Matrix4 &a, const Quaternion &b);
Matrix4 scale     (const Matrix4 &a, const Vector3 &b);

Matrix4 inverse (const Matrix4 &m);

Matrix4 make_transform    (const Vector3 &location, const Quaternion &rotation, const Vector3 &scale);
Matrix4 make_view         (const Vector3 &eye, const Vector3 &at, const Vector3 &up);
Matrix4 make_perspective  (f32 rfovy, f32 aspect, f32 n, f32 f);
Matrix4 make_orthographic (f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
