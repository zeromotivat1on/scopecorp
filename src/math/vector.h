#pragma once

// Basic vectors (2, 3, 4).

#define Vector2_zero Vector2(0,  0)

#define Vector3_zero    Vector3( 0,  0,  0)
#define Vector3_right   Vector3( 1,  0,  0)
#define Vector3_left    Vector3(-1,  0,  0)
#define Vector3_up      Vector3( 0,  1,  0)
#define Vector3_down    Vector3( 0, -1,  0)
#define Vector3_forward Vector3( 0,  0,  1)
#define Vector3_back    Vector3( 0,  0, -1)

#define Vector3_red    Vector3(1, 0, 0)
#define Vector3_green  Vector3(0, 1, 0)
#define Vector3_blue   Vector3(0, 0, 1)
#define Vector3_black  Vector3(0, 0, 0)
#define Vector3_white  Vector3(1, 1, 1)
#define Vector3_yellow Vector3(1, 1, 0)
#define Vector3_purple Vector3(1, 0, 1)

#define Vector4_red    Vector4(1, 0, 0, 1)
#define Vector4_green  Vector4(0, 1, 0, 1)
#define Vector4_blue   Vector4(0, 0, 1, 1)
#define Vector4_black  Vector4(0, 0, 0, 1)
#define Vector4_white  Vector4(1, 1, 1, 1)
#define Vector4_yellow Vector4(1, 1, 0, 1)
#define Vector4_purple Vector4(1, 0, 1, 1)

struct Vector2 {
    union {
        struct { f32 x, y; };
        f32 xy[2] = {0};
    };
    
    Vector2()             : x(0), y(0) {}
    Vector2(f32 a)        : x(a), y(a) {}
    Vector2(f32 x, f32 y) : x(x), y(y) {}
    
    inline f32 &operator[](s32 index)       { return xy[index]; }
    inline f32  operator[](s32 index) const { return xy[index]; }
};

struct Vector3 {
    union {
        struct { f32 x, y, z; };
        struct { Vector2 xy; f32  y; };
        struct { f32  x;  Vector2 yz; };
        f32 xyz[3] = {0};
    };
    
    Vector3()                     : x(0),   y(0),   z(0)   {}
    Vector3(f32 a)                : x(a),   y(a),   z(a)   {}
    Vector3(f32 x, f32 y)         : x(x),   y(y),   z(0)   {}
    Vector3(f32 x, f32 y, f32 z)  : x(x),   y(y),   z(z)   {}
    Vector3(const Vector2 &v, f32 z) : x(v.x), y(v.y), z(z)   {}
    Vector3(f32 x, const Vector2 &v) : x(x),   y(v.x), z(v.y) {}
    
    inline f32 &operator[](s32 index)       { return xyz[index]; }
    inline f32  operator[](s32 index) const { return xyz[index]; }
};

struct Vector4 {
    union {
        struct { f32 x, y, z, w; };
        struct { Vector3 xyz;   f32  w; };
        struct { Vector2 xy;    f32  z, w; };
        struct { Vector2 xy;    Vector2 zw; };
        struct { f32  x, y;  Vector2 zw; };
        struct { f32  x;     Vector3 yzw; };
        f32 xyzw[4] = {0};
    };
    
    Vector4()                             : x(0),   y(0),   z(0),   w(0)   {}
    Vector4(f32 a)                        : x(a),   y(a),   z(a),   w(a)   {}
    Vector4(f32 x, f32 y)                 : x(x),   y(y),   z(0),   w(0)   {}
    Vector4(f32 x, f32 y, f32 z)          : x(x),   y(y),   z(z),   w(0)   {}
    Vector4(f32 x, f32 y, f32 z, f32 w)   : x(x),   y(y),   z(z),   w(w)   {}
    Vector4(const Vector3 &v, f32 w)         : x(v.x), y(v.y), z(v.z), w(w)   {}
    Vector4(const Vector2 &v, f32 z, f32 w)  : x(v.x), y(v.y), z(z),   w(w)   {}
    Vector4(const Vector2 &a, const Vector2 &b) : x(a.x), y(a.y), z(b.x), w(b.y) {}
    Vector4(f32 x, f32 y, const Vector2 &v)  : x(x),   y(y),   z(v.x), w(v.y) {}
    Vector4(f32 x, const Vector3 &v)         : x(x),   y(v.x), z(v.y), w(v.z) {}
    
    inline f32 &operator[](s32 index)       { return xyzw[index]; }
    inline f32  operator[](s32 index) const { return xyzw[index]; }
};

Vector2 operator-(const Vector2 &a);
Vector3 operator-(const Vector3 &a);
Vector4 operator-(const Vector4 &a);

Vector2 operator+(const Vector2 &a, const Vector2 &b);
Vector3 operator+(const Vector3 &a, const Vector3 &b);
Vector4 operator+(const Vector4 &a, const Vector4 &b);

Vector2 operator-(const Vector2 &a, const Vector2 &b);
Vector3 operator-(const Vector3 &a, const Vector3 &b);
Vector4 operator-(const Vector4 &a, const Vector4 &b);

Vector2 operator*(const Vector2 &a, f32 b);
Vector3 operator*(const Vector3 &a, f32 b);
Vector4 operator*(const Vector4 &a, f32 b);

Vector2 operator*(f32 a, const Vector2 &b);
Vector3 operator*(f32 a, const Vector3 &b);
Vector4 operator*(f32 a, const Vector4 &b);

f32 operator*(const Vector2 &a, const Vector2 &b);
f32 operator*(const Vector3 &a, const Vector3 &b);
f32 operator*(const Vector4 &a, const Vector4 &b);

Vector2 operator/(const Vector2 &a, f32 b);
Vector3 operator/(const Vector3 &a, f32 b);
Vector4 operator/(const Vector4 &a, f32 b);

Vector2 operator/(const Vector2 &a, const Vector2 &b);
Vector3 operator/(const Vector3 &a, const Vector3 &b);
Vector4 operator/(const Vector4 &a, const Vector4 &b);

Vector2 &operator+=(Vector2 &a, const Vector2 &b);
Vector3 &operator+=(Vector3 &a, const Vector3 &b);
Vector4 &operator+=(Vector4 &a, const Vector4 &b);

Vector2 &operator-=(Vector2 &a, const Vector2 &b);
Vector3 &operator-=(Vector3 &a, const Vector3 &b);
Vector4 &operator-=(Vector4 &a, const Vector4 &b);

Vector2 &operator/=(Vector2 &a, f32 b);
Vector3 &operator/=(Vector3 &a, f32 b);
Vector4 &operator/=(Vector4 &a, f32 b);

Vector2 &operator/=(Vector2 &a, const Vector2 &b);
Vector3 &operator/=(Vector3 &a, const Vector3 &b);
Vector4 &operator/=(Vector4 &a, const Vector4 &b);

Vector2 &operator*=(Vector2 &a, f32 b);
Vector3 &operator*=(Vector3 &a, f32 b);
Vector4 &operator*=(Vector4 &a, f32 b);

bool operator==(const Vector2 &a, const Vector2 &b);
bool operator==(const Vector3 &a, const Vector3 &b);
bool operator==(const Vector4 &a, const Vector4 &b);

bool operator!=(const Vector2 &a, const Vector2 &b);
bool operator!=(const Vector3 &a, const Vector3 &b);
bool operator!=(const Vector4 &a, const Vector4 &b);
    
f32 length(const Vector2 &a);
f32 length(const Vector3 &a);
f32 length(const Vector4 &a);

f32 length_sqr(const Vector2 &a);
f32 length_sqr(const Vector3 &a);
f32 length_sqr(const Vector4 &a);

f32 dot(const Vector2 &a, const Vector2 &b);
f32 dot(const Vector3 &a, const Vector3 &b);
f32 dot(const Vector4 &a, const Vector4 &b);

Vector2 normalize(const Vector2 &v);
Vector3 normalize(const Vector3 &v);
Vector4 normalize(const Vector4 &v);

Vector2 direction(const Vector2 &a, const Vector2 &b);
Vector3 direction(const Vector3 &a, const Vector3 &b);
Vector4 direction(const Vector4 &a, const Vector4 &b);

Vector2 abs(const Vector2 &a);
Vector3 abs(const Vector3 &a);
Vector4 abs(const Vector4 &a);

bool equal(const Vector2 &a, const Vector2 &b, f32 c = F32_EPSILON);
bool equal(const Vector3 &a, const Vector3 &b, f32 c = F32_EPSILON);
bool equal(const Vector4 &a, const Vector4 &b, f32 c = F32_EPSILON);

Vector2 truncate(const Vector2 &a, f32 b);
Vector3 truncate(const Vector3 &a, f32 b);
Vector4 truncate(const Vector4 &a, f32 b);

String to_string(const Vector2 &v);
String to_string(const Vector3 &v);
String to_string(const Vector4 &v);

Vector3 cross(const Vector3 &a, const Vector3 &b);

Vector3 forward(f32 yaw, f32 pitch);
Vector3 right(const Vector3 &start, const Vector3 &end, const Vector3 &up);
