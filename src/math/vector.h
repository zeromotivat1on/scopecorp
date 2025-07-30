#pragma once

// Basic linal vectors (2, 3, 4).

#define vec2_zero vec2(0,  0)

#define vec3_zero    vec3( 0,  0,  0)
#define vec3_right   vec3( 1,  0,  0)
#define vec3_left    vec3(-1,  0,  0)
#define vec3_up      vec3( 0,  1,  0)
#define vec3_down    vec3( 0, -1,  0)
#define vec3_forward vec3( 0,  0,  1)
#define vec3_back    vec3( 0,  0, -1)

#define vec3_red    vec3(1, 0, 0)
#define vec3_green  vec3(0, 1, 0)
#define vec3_blue   vec3(0, 0, 1)
#define vec3_black  vec3(0, 0, 0)
#define vec3_white  vec3(1, 1, 1)
#define vec3_yellow vec3(1, 1, 0)
#define vec3_purple vec3(1, 0, 1)

#define vec4_red    vec4(1, 0, 0, 1)
#define vec4_green  vec4(0, 1, 0, 1)
#define vec4_blue   vec4(0, 0, 1, 1)
#define vec4_black  vec4(0, 0, 0, 1)
#define vec4_white  vec4(1, 1, 1, 1)
#define vec4_yellow vec4(1, 1, 0, 1)
#define vec4_purple vec4(1, 0, 1, 1)

struct vec2 {
    union {
        struct { f32 x, y; };
        f32 xy[2] = {0};
    };
    
    vec2()             : x(0), y(0) {}
    vec2(f32 a)        : x(a), y(a) {}
    vec2(f32 x, f32 y) : x(x), y(y) {}
    
    inline f32 &operator[](s32 index)       { return xy[index]; }
    inline f32  operator[](s32 index) const { return xy[index]; }
};

struct vec3 {
    union {
        struct { f32 x, y, z; };
        struct { vec2 xy; f32  y; };
        struct { f32  x;  vec2 yz; };
        f32 xyz[3] = {0};
    };
    
    vec3()                     : x(0),   y(0),   z(0)   {}
    vec3(f32 a)                : x(a),   y(a),   z(a)   {}
    vec3(f32 x, f32 y)         : x(x),   y(y),   z(0)   {}
    vec3(f32 x, f32 y, f32 z)  : x(x),   y(y),   z(z)   {}
    vec3(const vec2 &v, f32 z) : x(v.x), y(v.y), z(z)   {}
    vec3(f32 x, const vec2 &v) : x(x),   y(v.x), z(v.y) {}
    
    inline f32 &operator[](s32 index)       { return xyz[index]; }
    inline f32  operator[](s32 index) const { return xyz[index]; }
};

struct vec4 {
    union {
        struct { f32 x, y, z, w; };
        struct { vec3 xyz;   f32  w; };
        struct { vec2 xy;    f32  z, w; };
        struct { vec2 xy;    vec2 zw; };
        struct { f32  x, y;  vec2 zw; };
        struct { f32  x;     vec3 yzw; };
        f32 xyzw[4] = {0};
    };
    
    vec4()                             : x(0),   y(0),   z(0),   w(0)   {}
    vec4(f32 a)                        : x(a),   y(a),   z(a),   w(a)   {}
    vec4(f32 x, f32 y)                 : x(x),   y(y),   z(0),   w(0)   {}
    vec4(f32 x, f32 y, f32 z)          : x(x),   y(y),   z(z),   w(0)   {}
    vec4(f32 x, f32 y, f32 z, f32 w)   : x(x),   y(y),   z(z),   w(w)   {}
    vec4(const vec3 &v, f32 w)         : x(v.x), y(v.y), z(v.z), w(w)   {}
    vec4(const vec2 &v, f32 z, f32 w)  : x(v.x), y(v.y), z(z),   w(w)   {}
    vec4(const vec2 &a, const vec2 &b) : x(a.x), y(a.y), z(b.x), w(b.y) {}
    vec4(f32 x, f32 y, const vec2 &v)  : x(x),   y(y),   z(v.x), w(v.y) {}
    vec4(f32 x, const vec3 &v)         : x(x),   y(v.x), z(v.y), w(v.z) {}
    
    inline f32 &operator[](s32 index)       { return xyzw[index]; }
    inline f32  operator[](s32 index) const { return xyzw[index]; }
};

vec2 operator-(const vec2 &a);
vec3 operator-(const vec3 &a);
vec4 operator-(const vec4 &a);

vec2 operator+(const vec2 &a, const vec2 &b);
vec3 operator+(const vec3 &a, const vec3 &b);
vec4 operator+(const vec4 &a, const vec4 &b);

vec2 operator-(const vec2 &a, const vec2 &b);
vec3 operator-(const vec3 &a, const vec3 &b);
vec4 operator-(const vec4 &a, const vec4 &b);

vec2 operator*(const vec2 &a, f32 b);
vec3 operator*(const vec3 &a, f32 b);
vec4 operator*(const vec4 &a, f32 b);

vec2 operator*(f32 a, const vec2 &b);
vec3 operator*(f32 a, const vec3 &b);
vec4 operator*(f32 a, const vec4 &b);

f32 operator*(const vec2 &a, const vec2 &b);
f32 operator*(const vec3 &a, const vec3 &b);
f32 operator*(const vec4 &a, const vec4 &b);

vec2 operator/(const vec2 &a, f32 b);
vec3 operator/(const vec3 &a, f32 b);
vec4 operator/(const vec4 &a, f32 b);

vec2 operator/(const vec2 &a, const vec2 &b);
vec3 operator/(const vec3 &a, const vec3 &b);
vec4 operator/(const vec4 &a, const vec4 &b);

vec2 &operator+=(vec2 &a, const vec2 &b);
vec3 &operator+=(vec3 &a, const vec3 &b);
vec4 &operator+=(vec4 &a, const vec4 &b);

vec2 &operator-=(vec2 &a, const vec2 &b);
vec3 &operator-=(vec3 &a, const vec3 &b);
vec4 &operator-=(vec4 &a, const vec4 &b);

vec2 &operator/=(vec2 &a, f32 b);
vec3 &operator/=(vec3 &a, f32 b);
vec4 &operator/=(vec4 &a, f32 b);

vec2 &operator/=(vec2 &a, const vec2 &b);
vec3 &operator/=(vec3 &a, const vec3 &b);
vec4 &operator/=(vec4 &a, const vec4 &b);

vec2 &operator*=(vec2 &a, f32 b);
vec3 &operator*=(vec3 &a, f32 b);
vec4 &operator*=(vec4 &a, f32 b);

bool operator==(const vec2 &a, const vec2 &b);
bool operator==(const vec3 &a, const vec3 &b);
bool operator==(const vec4 &a, const vec4 &b);

bool operator!=(const vec2 &a, const vec2 &b);
bool operator!=(const vec3 &a, const vec3 &b);
bool operator!=(const vec4 &a, const vec4 &b);
    
f32 length(const vec2 &a);
f32 length(const vec3 &a);
f32 length(const vec4 &a);

f32 length_sqr(const vec2 &a);
f32 length_sqr(const vec3 &a);
f32 length_sqr(const vec4 &a);

f32 dot(const vec2 &a, const vec2 &b);
f32 dot(const vec3 &a, const vec3 &b);
f32 dot(const vec4 &a, const vec4 &b);

vec2 normalize(const vec2 &v);
vec3 normalize(const vec3 &v);
vec4 normalize(const vec4 &v);

vec2 direction(const vec2 &a, const vec2 &b);
vec3 direction(const vec3 &a, const vec3 &b);
vec4 direction(const vec4 &a, const vec4 &b);

vec2 Abs(const vec2 &a);
vec3 Abs(const vec3 &a);
vec4 Abs(const vec4 &a);

bool equal(const vec2 &a, const vec2 &b, f32 c = F32_EPSILON);
bool equal(const vec3 &a, const vec3 &b, f32 c = F32_EPSILON);
bool equal(const vec4 &a, const vec4 &b, f32 c = F32_EPSILON);

vec2 truncate(const vec2 &a, f32 b);
vec3 truncate(const vec3 &a, f32 b);
vec4 truncate(const vec4 &a, f32 b);
    
// Uses static 2d buffer internally, for debug purposes only.
const char *to_string(const vec2 &v);
const char *to_string(const vec3 &v);
const char *to_string(const vec4 &v);

vec3 cross(const vec3 &a, const vec3 &b);

vec3 forward(f32 yaw, f32 pitch);
vec3 right(const vec3 &start, const vec3 &end, const vec3 &up);
