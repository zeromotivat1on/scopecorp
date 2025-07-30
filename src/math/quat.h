#pragma once

struct vec3;
struct mat3;
struct mat4;

struct quat {
    union {
        struct { f32 x, y, z, w; };
        f32 xyzw[4];
    };
    
	quat()                           : x(0), y(0), z(0), w(1) {}
	quat(f32 a)                      : x(a), y(a), z(a), w(1) {}
	quat(f32 a, f32 b, f32 c, f32 d) : x(a), y(b), z(c), w(d) {}

    inline f32 &operator[](s32 index)       { return xyzw[index]; }
    inline f32  operator[](s32 index) const { return xyzw[index]; }
};
    
quat  operator-(const quat &a);
quat  operator+(const quat &a, const quat &b);
quat  operator-(const quat &a, const quat &b);
quat  operator*(const quat &a, const quat &b);
vec3  operator*(const quat &a, const vec3 &b);
quat  operator*(const quat &a, f32 b);
quat &operator+=(quat &a, const quat &b);
quat &operator-=(quat &a, const quat &b);
quat &operator*=(quat &a, const quat &b);
quat &operator*=(quat &a, f32 b);

bool operator==(const quat &a, const quat &b);
bool operator!=(const quat &a, const quat &b);

quat operator*(const f32 a, const quat &b);
vec3 operator*(const vec3 &a, const quat &b);

bool equal(const quat &a, const quat &b, f32 c = F32_EPSILON);

f32	 length(const quat &a);
f32	 calc_w(const quat &a);
quat inverse(const quat &a);
quat normalize(const quat &a);
vec3 forward(const quat &q);

const char *to_string(const quat &q);

quat quat_from_axis_angle(const vec3 &axes, f32 deg);
