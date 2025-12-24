#pragma once

struct Vector3;
struct Matrix3;
struct Matrix4;

struct Quaternion {
    union {
        struct { f32 x, y, z, w; };
        f32 xyzw[4];
    };
    
	Quaternion()                           : x(0), y(0), z(0), w(1) {}
	Quaternion(f32 a)                      : x(a), y(a), z(a), w(1) {}
	Quaternion(f32 a, f32 b, f32 c, f32 d) : x(a), y(b), z(c), w(d) {}

    inline f32 &operator[](s32 index)       { return xyzw[index]; }
    inline f32  operator[](s32 index) const { return xyzw[index]; }
};
    
Quaternion  operator-(const Quaternion &a);
Quaternion  operator+(const Quaternion &a, const Quaternion &b);
Quaternion  operator-(const Quaternion &a, const Quaternion &b);
Quaternion  operator*(const Quaternion &a, const Quaternion &b);
Vector3  operator*(const Quaternion &a, const Vector3 &b);
Quaternion  operator*(const Quaternion &a, f32 b);
Quaternion &operator+=(Quaternion &a, const Quaternion &b);
Quaternion &operator-=(Quaternion &a, const Quaternion &b);
Quaternion &operator*=(Quaternion &a, const Quaternion &b);
Quaternion &operator*=(Quaternion &a, f32 b);

bool operator==(const Quaternion &a, const Quaternion &b);
bool operator!=(const Quaternion &a, const Quaternion &b);

Quaternion operator*(const f32 a, const Quaternion &b);
Vector3 operator*(const Vector3 &a, const Quaternion &b);

bool equal(const Quaternion &a, const Quaternion &b, f32 c = F32_EPSILON);

f32	 length(const Quaternion &a);
f32	 calc_w(const Quaternion &a);
Quaternion inverse(const Quaternion &a);
Quaternion normalize(const Quaternion &a);
Vector3 forward(const Quaternion &q);

String to_string(const Quaternion &q);

Quaternion make_quaternion_from_axis_angle(const Vector3 &axes, f32 deg);
