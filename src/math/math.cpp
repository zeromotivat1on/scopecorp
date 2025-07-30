#include "pch.h"
#include "log.h"
#include "stb_sprintf.h"

#include "math/math_basic.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/quat.h"

#include <math.h>

// basic

s32 Abs(s32 n) {
	const s32 sign = n >> 31;
	return (n ^ sign) - sign;
}

s64 Abs(s64 n) {
	const s64 sign = n >> 63;
	return (n ^ sign) - sign;
}

f32 Abs(f32 n) {
	s32 tmp = *(s32 *)(&n);
	tmp &= 0x7FFFFFFF; // clear sign bit
	return *(f32 *)(&tmp);
}

f32 Sqrt(f32 n) {
	return sqrtf(n);
}

f32 Sqrtr(f32 n) {
	return 1 / sqrtf(n);
}

f32 Rad(f32 d) {
	return d * (PI / 180.0f);
}

f32 Deg(f32 r) {
	return r * (180.0f / PI);
}

f32 Cos(f32 r) {
	return cosf(r);
}

f32 Sin(f32 r) {
	return sinf(r);
}

f32 Tan(f32 r) {
	return tanf(r);
}

// vec2

vec2 operator-(const vec2 &a)                 { return vec2(-a.x, -a.y); }
vec2 operator+(const vec2 &a, const vec2 &b)  { return vec2(a.x + b.x, a.y + b.y); }
vec2 operator-(const vec2 &a, const vec2 &b)  { return vec2(a.x - b.x, a.y - b.y); }
vec2 operator*(const vec2 &a, f32 b)          { return vec2(a.x * b, a.y * b); }
vec2 operator*(f32 a, const vec2 &b)          { return b * a; }
f32  operator*(const vec2 &a, const vec2 &b)  { return dot(a, b); }
vec2 operator/(const vec2 &a, f32 b)          { return vec2(a.x / b, a.y / b); }
vec2 operator/(const vec2 &a, const vec2 &b)  { return vec2(a.x / b.x, a.y / b.y); }
vec2 &operator+=(vec2 &a, const vec2 &b)      { a = a + b; return a; }
vec2 &operator-=(vec2 &a, const vec2 &b)      { a = a - b; return a; }
vec2 &operator/=(vec2 &a, f32 b)              { a = a / b; return a; }
vec2 &operator/=(vec2 &a, const vec2 &b)      { a = a / b; return a; }
vec2 &operator*=(vec2 &a, f32 b)              { a = a * b; return a; }
bool operator==(const vec2 &a, const vec2 &b) { return equal(a, b); }
bool operator!=(const vec2 &a, const vec2 &b) { return !(a == b); }

f32  length(const vec2 &a)                   { return Sqrt(length_sqr(a)); }
f32  length_sqr(const vec2 &a)               { return a.x * a.x + a.y * a.y; }
f32  dot(const vec2 &a, const vec2 &b)       { return a.x * b.x + a.y * b.y; }
vec2 normalize(const vec2 &a)                { return a / length(a); }
vec2 direction(const vec2 &a, const vec2 &b) { return normalize(b - a); }
vec2 Abs(const vec2 &a)                      { return vec2(Abs(a.x), Abs(a.y)); }

bool equal(const vec2 &a, const vec2 &b, f32 c) {
    return Abs(a.x - b.x) <= c && Abs(a.y - b.y) <= c;
}

vec2 truncate(const vec2 &a, f32 b) {
    if (b == 0.0f) return vec2{0.0f, 0.0f};
    const f32 lensqr = length_sqr(a);
    if (lensqr > b * b) { const f32 s = b * Sqrtr(lensqr); return a * s; }
    return a;
}

const char *to_string(const vec2 &a) {
    static char buffers[4][32];
    static s32 index = 0;
    index = (index + 1) % 4;

    char* buffer = buffers[index];
    stbsp_snprintf(buffer, 32, "(%.3f %.3f)", a.x, a.y);
    return buffer;
}

// vec3

vec3 operator-(const vec3 &a)                 { return vec3(-a.x, -a.y, -a.z); }
vec3 operator+(const vec3 &a, const vec3 &b)  { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
vec3 operator-(const vec3 &a, const vec3 &b)  { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
vec3 operator*(const vec3 &a, f32 b)          { return vec3(a.x * b, a.y * b, a.z * b); }
vec3 operator*(f32 a, const vec3 &b)          { return b * a; }
f32  operator*(const vec3 &a, const vec3 &b)  { return dot(a, b); }
vec3 operator/(const vec3 &a, f32 b)          { return vec3(a.x / b, a.y / b, a.z / b); }
vec3 operator/(const vec3 &a, const vec3 &b)  { return vec3(a.x / b.x, a.y / b.y, a.z / b.z); }
vec3 &operator+=(vec3 &a, const vec3 &b)      { a = a + b; return a; }
vec3 &operator-=(vec3 &a, const vec3 &b)      { a = a - b; return a; }
vec3 &operator/=(vec3 &a, f32 b)              { a = a / b; return a; }
vec3 &operator/=(vec3 &a, const vec3 &b)      { a = a / b; return a; }
vec3 &operator*=(vec3 &a, f32 b)              { a = a * b; return a; }
bool operator==(const vec3 &a, const vec3 &b) { return equal(a, b); }
bool operator!=(const vec3 &a, const vec3 &b) { return !(a == b); }

f32  length(const vec3 &a)                   { return Sqrt(length_sqr(a)); }
f32  length_sqr(const vec3 &a)               { return a.x * a.x + a.y * a.y + a.z * a.z; }
f32  dot(const vec3 &a, const vec3 &b)       { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3 normalize(const vec3 &a)                { return a / length(a); }
vec3 direction(const vec3 &a, const vec3 &b) { return normalize(b - a); }
vec3 Abs(const vec3 &a)                      { return vec3(Abs(a.x), Abs(a.y), Abs(a.z)); }

bool equal(const vec3 &a, const vec3 &b, f32 c) {
    return Abs(a.x - b.x) <= c && Abs(a.y - b.y) <= c && Abs(a.z - b.z) <= c;
}

vec3 truncate(const vec3&a, f32 b) {
    if (b == 0.0f) return vec3{0.0f, 0.0f, 0.0f};
    const f32 lensqr = length_sqr(a);
    if (lensqr > b * b) { const f32 s = b * Sqrtr(lensqr); return a * s; }
    return a;
}

vec3 cross(const vec3 &a, const vec3 &b) {
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

const char* to_string(const vec3 &a) {
    static char buffers[4][32];
    static s32 index = 0;
    index = (index + 1) % 4;

    char* buffer = buffers[index];
    stbsp_snprintf(buffer, 32, "(%.3f %.3f %.3f)", a.x, a.y, a.z);
    return buffer;
}

vec3 forward(f32 yaw, f32 pitch) {
    const f32 ycos = Cos(Rad(yaw));
    const f32 ysin = Sin(Rad(yaw));
    const f32 pcos = Cos(Rad(pitch));
    const f32 psin = Sin(Rad(pitch));
    return normalize(vec3(ycos * pcos, psin, ysin * pcos));
}

vec3 right(const vec3 &start, const vec3 &end, const vec3 &up) {
    return normalize(cross(up, end - start));
}

// vec4

vec4 operator-(const vec4 &a)                 { return vec4(-a.x, -a.y, -a.z, -a.w); }
vec4 operator+(const vec4 &a, const vec4 &b)  { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
vec4 operator-(const vec4 &a, const vec4 &b)  { return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
vec4 operator*(const vec4 &a, f32 b)          { return vec4(a.x * b, a.y * b, a.z * b, a.w * b); }
vec4 operator*(f32 a, const vec4 &b)          { return b * a; }
f32  operator*(const vec4 &a, const vec4 &b)  { return dot(a, b); }
vec4 operator/(const vec4 &a, f32 b)          { return vec4(a.x / b, a.y / b, a.z / b, a.w / b); }
vec4 operator/(const vec4 &a, const vec4 &b)  { return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }
vec4 &operator+=(vec4 &a, const vec4 &b)      { a = a + b; return a; }
vec4 &operator-=(vec4 &a, const vec4 &b)      { a = a - b; return a; }
vec4 &operator/=(vec4 &a, f32 b)              { a = a / b; return a; }
vec4 &operator/=(vec4 &a, const vec4 &b)      { a = a / b; return a; }
vec4 &operator*=(vec4 &a, f32 b)              { a = a * b; return a; }
bool operator==(const vec4 &a, const vec4 &b) { return equal(a, b); }
bool operator!=(const vec4 &a, const vec4 &b) { return !(a == b); }

f32  length(const vec4 &a)                   { return Sqrt(length_sqr(a)); }
f32  length_sqr(const vec4 &a)               { return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w; }
f32  dot(const vec4 &a, const vec4 &b)       { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
vec4 normalize(const vec4 &a)                { return a / length(a); }
vec4 direction(const vec4 &a, const vec4 &b) { return normalize(b - a); }
vec4 Abs(const vec4 &a)                      { return vec4(Abs(a.x), Abs(a.y), Abs(a.z), Abs(a.w)); }

bool equal(const vec4 &a, const vec4 &b, f32 c) {
    return Abs(a.x - b.x) <= c && Abs(a.y - b.y) <= c && Abs(a.z - b.z) <= c && Abs(a.w - b.w) <= c;
}

vec4 truncate(const vec4&a, f32 b) {
    if (b == 0.0f) return vec4{0.0f, 0.0f, 0.0f, 0.0f};
    const f32 lensqr = length_sqr(a);
    if (lensqr > b * b) { const f32 s = b * Sqrtr(lensqr); return a * s; }
    return a;
}

const char *to_string(const vec4 &a) {
    static char buffers[4][32];
    static s32 index = 0;
    index = (index + 1) % 4;

    char* buffer = buffers[index];
    stbsp_snprintf(buffer, 32, "(%.3f %.3f %.3f %.3f)", a.x, a.y, a.z, a.w);
    return buffer;
}

// mat2

mat2 mat2_identity() {
	return mat2(vec2(1, 0), vec2(0, 1));
}

mat2 operator-(const mat2 &a) { return mat2(-a[0], -a[1]); }
mat2 operator+(const mat2 &a, const mat2 &b) { return mat2(a[0] + b[0], a[1] + b[1]); }
mat2 operator-(const mat2 &a, const mat2 &b) { return mat2(a[0] - b[0], a[1] - b[1]); }

mat2 operator*(const mat2 &a, const mat2 &b) {
	return mat2(a[0][0] * b[0][0] + a[0][1] * b[1][0],
                a[0][0] * b[0][1] + a[0][1] * b[1][1],
                a[1][0] * b[0][0] + a[1][1] * b[1][0],
                a[1][0] * b[0][1] + a[1][1] * b[1][1]);
}

mat2 operator*(const mat2 &a, f32 b) { return mat2(a[0] * b, a[1] * b); }

vec2 operator*(const mat2 &a, const vec2 &b) {
	return vec2(a[0][0] * b[0] + a[0][1] * b[1],
                a[1][0] * b[0] + a[1][1] * b[1]);
}

mat2 &operator+=(mat2 &a, const mat2 &b) { a = a + b; return a; }
mat2 &operator-=(mat2 &a, const mat2 &b) { a = a - b; return a; }
mat2 &operator*=(mat2 &a, const mat2 &b) { a = a * b; return a; }
mat2 &operator*=(mat2 &a, f32 b)         { a = a * b; return a; }

bool operator==(const mat2 &a, const mat2 &b) { return equal(a, b); }
bool operator!=(const mat2 &a, const mat2 &b) { return !(a == b); }

mat2 operator*(f32 a, const mat2 &b)         { return b * a; }
vec2 operator*(const vec2 &a, const mat2 &b) { return b * a; }
vec2 &operator*=(vec2 &a, const mat2 &b)     { a = a * b; return a; }

bool equal(const mat2 &a, const mat2 &b, f32 c) {
	return equal(a[0], b[0], c) && equal(a[1], b[1], c);
}

bool identity (const mat2 &a, f32 b) { return equal(a, mat2_identity(), b); }
bool symmetric(const mat2 &a, f32 b) { return Abs(a[0][1] - a[1][0]) < b; }
bool diagonal (const mat2 &a, f32 b) { return Abs(a[0][1]) <= b && Abs(a[1][0]) <= b; }

mat2 transpose(const mat2 &a) { return mat2(a[0][0], a[1][0], a[0][1], a[1][1]); }
f32  trace(const mat2 &a)     { return a[0][0] + a[1][1]; }

// mat3

mat3::mat3(const quat &a) {
    const f32 x2 = a.x + a.x;
    const f32 y2 = a.y + a.y;
    const f32 z2 = a.z + a.z;

    const f32 xx = a.x * x2;
    const f32 xy = a.x * y2;
    const f32 xz = a.x * z2;

    const f32 yy = a.y * y2;
    const f32 yz = a.y * z2;
    const f32 zz = a.z * z2;

    const f32 wx = a.w * x2;
    const f32 wy = a.w * y2;
    const f32 wz = a.w * z2;

    v[0][0] = 1.0f - (yy + zz);
    v[1][0] = xy - wz;
    v[2][0] = xz + wy;

    v[0][1] = xy + wz;
    v[1][1] = 1.0f - (xx + zz);
    v[2][1] = yz - wx;

    v[0][2] = xz - wy;
    v[1][2] = yz + wx;
    v[2][2] = 1.0f - (xx + yy);
}

mat3 mat3_identity() {
	return mat3(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
}

mat3 operator-(const mat3 &a) { return mat3(-a[0], -a[1], -a[2]); }
mat3 operator+(const mat3 &a, const mat3 &b) { return mat3(a[0] + b[0], a[1] + b[1], a[2] + b[2]); }
mat3 operator-(const mat3 &a, const mat3 &b) { return mat3(a[0] - b[0], a[1] - b[1], a[2] - b[2]); }

mat3 operator*(const mat3 &a, const mat3 &b) {
	mat3 r;
    
	f32 *pr = r.e;
	const f32* pa = a.e;
    const f32* pb = b.e;

	for (u8 i = 0; i < 3; ++i) {
		for (u8 j = 0; j < 3; ++j) {
			*pr = pa[0] * pb[0 * 3 + j]
                + pa[1] * pb[1 * 3 + j]
                + pa[2] * pb[2 * 3 + j];
			pr += 1;
		}
		pa += 3;
	}

	return r;
}

mat3 operator*(const mat3 &a, f32 b) { return mat3(a[0] * b, a[1] * b, a[2] * b); }

vec3 operator*(const mat3 &a, const vec3 &b) {
	return vec3(a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2],
                a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2],
                a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2]);
}

mat3 &operator+=(mat3 &a, const mat3 &b) { a = a + b; return a; }
mat3 &operator-=(mat3 &a, const mat3 &b) { a = a - b; return a; }
mat3 &operator*=(mat3 &a, const mat3 &b) { a = a * b; return a; }
mat3 &operator*=(mat3 &a, f32 b)         { a = a * b; return a; }

bool operator==(const mat3 &a, const mat3 &b) { return equal(a, b); }
bool operator!=(const mat3 &a, const mat3 &b) { return !(a == b); }

mat3 operator*(f32 a, const mat3 &b)         { return b * a; }
vec3 operator*(const vec3 &a, const mat3 &b) { return b * a; }
vec3 &operator*=(vec3 &a, const mat3 &b)     { a = a * b; return a; }

bool equal(const mat3 &a, const mat3 &b, f32 c) {
	return equal(a[0], b[0], c) && equal(a[1], b[1], c) && equal(a[2], b[2], c);
}

bool identity(const mat3 &a, f32 b) { return equal(a, mat3_identity(), b); }

bool symmetric(const mat3 &a, f32 b) {
    return Abs(a[0][1] - a[1][0]) <= b
        && Abs(a[0][2] - a[2][0]) <= b
        && Abs(a[1][2] - a[2][1]) <= b;
}

bool diagonal(const mat3 &a, f32 b) {
    return Abs(a[0][1]) <= b
        && Abs(a[0][2]) <= b
        && Abs(a[1][0]) <= b
        && Abs(a[1][2]) <= b
        && Abs(a[2][0]) <= b
        && Abs(a[2][1]) <= b;
}

bool rotated(const mat3 &a) {
	return a[0][1] != 0.0f || a[0][2] != 0.0f
        || a[1][0] != 0.0f || a[1][2] != 0.0f
        || a[2][0] != 0.0f || a[2][1] != 0.0f;
}

mat3 transpose(const mat3 &a) {
	return mat3(a[0][0], a[1][0], a[2][0],
                a[0][1], a[1][1], a[2][1],
                a[0][2], a[1][2], a[2][2]);
}

f32 trace(const mat3 &a) { return a[0][0] + a[1][1] + a[2][2]; }

// mat4

mat4 mat4_identity() {
	return mat4(vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
}

mat4 operator-(const mat4 &a) { return mat4(-a[0], -a[1], -a[2], -a[3]); }
mat4 operator+(const mat4 &a, const mat4 &b) { return mat4(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]); }
mat4 operator-(const mat4 &a, const mat4 &b) { return mat4(a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]); }

mat4 operator*(const mat4 &a, const mat4 &b) {
	mat4 r;
    
	f32 *pr = r.e;
	const f32* pa = a.e;
    const f32* pb = b.e;

	for (u8 i = 0; i < 4; ++i) {
		for (u8 j = 0; j < 4; ++j) {
			*pr = pa[0] * pb[0 * 4 + j]
                + pa[1] * pb[1 * 4 + j]
                + pa[2] * pb[2 * 4 + j]
                + pa[3] * pb[3 * 4 + j];
			pr += 1;
		}
		pa += 4;
	}

	return r;
}

mat4 operator*(const mat4 &a, f32 b) { return mat4(a[0] * b, a[1] * b, a[2] * b, a[3] * b); }

vec4 operator*(const mat4 &a, const vec4 &b) {
	return vec4(a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3] * b[3],
                a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3] * b[3],
                a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3] * b[3],
                a[3][0] * b[0] + a[3][1] * b[1] + a[3][2] * b[2] + a[3][3] * b[3]);
}

vec3 operator*(const mat4 &a, const vec3 &b) {
	const f32 s = a[3][0] * b[0] + a[3][1] * b[1] + a[3][2] * b[2] + a[3][3];

	if (s == 0.0f) return vec3_zero;

	if (s == 1.0f) {
		return vec3(a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3],
                    a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3],
                    a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3]);
	}

	const f32 sr = 1.0f / s;
    return vec3((a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3]) * sr,
                (a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3]) * sr,
                (a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3]) * sr);
}

mat4 &operator+=(mat4 &a, const mat4 &b) { a = a + b; return a; }
mat4 &operator-=(mat4 &a, const mat4 &b) { a = a - b; return a; }
mat4 &operator*=(mat4 &a, const mat4 &b) { a = a * b; return a; }
mat4 &operator*=(mat4 &a, f32 b)         { a = a * b; return a; }

bool operator==(const mat4 &a, const mat4 &b) { return equal(a, b); }
bool operator!=(const mat4 &a, const mat4 &b) { return !(a == b); }

mat4 operator*(f32 a, const mat4 &b)         { return b * a; }
vec3 operator*(const vec3 &a, const mat4 &b) { return b * a; }
vec4 operator*(const vec4 &a, const mat4 &b) { return b * a; }

vec3 &operator*=(vec3 &a, const mat4 &b) { a = b * a; return a; }
vec4 &operator*=(vec4 &a, const mat4 &b) { a = b * a; return a; }

bool equal(const mat4 &a, const mat4 &b, f32 c) {
	return equal(a[0], b[0], c)
        && equal(a[1], b[1], c)
        && equal(a[2], b[2], c)
        && equal(a[3], b[3], c);
}

bool identity(const mat4 &a, f32 b) { return equal(a, mat4_identity(), b); }

bool symmetric(const mat4 &a, f32 b) {
	for (s32 i = 1; i < 4; ++i)
		for (s32 j = 0; j < i; ++j)
			if (Abs(a[i][j] - a[j][i]) > b) return false;
	return true;
}

bool diagonal(const mat4 &a, f32 b) {
	for (s32 i = 0; i < 4; ++i)
		for (s32 j = 0; j < 4; ++j) 
			if (i != j && Abs(a[i][j]) > b)  return false;
	return true;
}

bool rotated(const mat4 &a) {
	return a[0][1] != 0.0f || a[0][2] != 0.0f
        || a[1][0] != 0.0f || a[1][2] != 0.0f
        || a[2][0] != 0.0f || a[2][1] != 0.0f;
}

mat4 &identity(mat4 &a) { a = mat4_identity(); return a; }

mat4 transpose(const mat4 &a) {
	mat4 r;
	for(s32 i = 0; i < 4; ++i)
		for(s32 j = 0; j < 4; ++j)
			r[i][j] = a[j][i];

	return r;
}

f32 trace(const mat4 &a) { return a[0][0] + a[1][1] + a[2][2] + a[3][3]; }

mat4 translate(const mat4 &a, const vec3 &b) {
    mat4 r = a; r[3][0] = b.x; r[3][1] = b.y; r[3][2] = b.z; return r;
}

mat4 rotate(const mat4 &a, const mat4 &b) { return a * b; }
mat4 rotate(const mat4 &a, const quat &b) { return a * mat4(b); }

mat4 scale(const mat4 &a, const vec3 &b) {
    mat4 r = a; r[0][0] = b.x; r[1][1] = b.y; r[2][2] = b.z; return r;
}

mat4 mat4_transform(const vec3 &t, const quat &r, const vec3 &s) {
    return translate(rotate(scale(mat4_identity(), s), r), t);
}

mat4 mat4_view(const vec3 &eye, const vec3 &at, const vec3 &up) {
	const vec3 f = normalize(at - eye);
	const vec3 r = normalize(cross(up, f));
	const vec3 u = cross(f, r);

	mat4 a;

	a[0][0] = r.x;
	a[1][0] = r.y;
	a[2][0] = r.z;
	a[3][0] = -dot(r, eye);

	a[0][1] = u.x;
	a[1][1] = u.y;
	a[2][1] = u.z;
	a[3][1] = -dot(u, eye);

	a[0][2] = -f.x;
	a[1][2] = -f.y;
	a[2][2] = -f.z;
	a[3][2] = dot(f, eye);

	a[0][3] = 0.0f;
	a[1][3] = 0.0f;
	a[2][3] = 0.0f;
	a[3][3] = 1.0f;

	return a;
}

mat4 mat4_perspective(f32 rfovy, f32 aspect, f32 n, f32 f) {
	const f32 tan_half_fovy = Tan(rfovy * 0.5f);

	mat4 a;
	a[0][0] = 1.0f / (aspect * tan_half_fovy);	// x-axis scaling
	a[1][1] = 1.0f / tan_half_fovy;			// y-axis scaling
	a[2][2] = -(f + n) / (f - n);				// z-axis scaling
	a[2][3] = -1.0f;							// rh perspective divide
	a[3][2] = -(2.0f * f * n) / (f - n);		// z-axis translation

	return a;
}

mat4 mat4_orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	mat4 a;
	a[0][0] =  2.0f / (r - l);
	a[1][1] =  2.0f / (t - b);
	a[2][2] = -2.0f / (f - n);
	a[3][0] = -(r + l) / (r - l);
	a[3][1] = -(t + b) / (t - b);
	a[3][2] = -(f + n) / (f - n);
	a[3][3] =  1.0f;
	return a;
}

mat4 inverse(const mat4 &m) {
    mat4 minv = m;
    
	// 2x2 sub-determinants required to calculate 4x4 determinant
	const f32 det2_01_01 = minv[0][0] * minv[1][1] - minv[0][1] * minv[1][0];
	const f32 det2_01_02 = minv[0][0] * minv[1][2] - minv[0][2] * minv[1][0];
	const f32 det2_01_03 = minv[0][0] * minv[1][3] - minv[0][3] * minv[1][0];
	const f32 det2_01_12 = minv[0][1] * minv[1][2] - minv[0][2] * minv[1][1];
	const f32 det2_01_13 = minv[0][1] * minv[1][3] - minv[0][3] * minv[1][1];
	const f32 det2_01_23 = minv[0][2] * minv[1][3] - minv[0][3] * minv[1][2];

	// 3x3 sub-determinants required to calculate 4x4 determinant
	const f32 det3_201_012 = minv[2][0] * det2_01_12 - minv[2][1] * det2_01_02 + minv[2][2] * det2_01_01;
	const f32 det3_201_013 = minv[2][0] * det2_01_13 - minv[2][1] * det2_01_03 + minv[2][3] * det2_01_01;
	const f32 det3_201_023 = minv[2][0] * det2_01_23 - minv[2][2] * det2_01_03 + minv[2][3] * det2_01_02;
	const f32 det3_201_123 = minv[2][1] * det2_01_23 - minv[2][2] * det2_01_13 + minv[2][3] * det2_01_12;

	const f32 det = ( - det3_201_123 * minv[3][0] + det3_201_023 * minv[3][1] - det3_201_013 * minv[3][2] + det3_201_012 * minv[3][3] );

	if (Abs(det) < MATRIX_INV_EPSILON ) {
        error("Failed to inverse mat4");
		return mat4_identity();
	}

	const f32 det_inv = 1.0f / det;

	// remaining 2x2 sub-determinants
	const f32 det2_03_01 = minv[0][0] * minv[3][1] - minv[0][1] * minv[3][0];
	const f32 det2_03_02 = minv[0][0] * minv[3][2] - minv[0][2] * minv[3][0];
	const f32 det2_03_03 = minv[0][0] * minv[3][3] - minv[0][3] * minv[3][0];
	const f32 det2_03_12 = minv[0][1] * minv[3][2] - minv[0][2] * minv[3][1];
	const f32 det2_03_13 = minv[0][1] * minv[3][3] - minv[0][3] * minv[3][1];
	const f32 det2_03_23 = minv[0][2] * minv[3][3] - minv[0][3] * minv[3][2];

	const f32 det2_13_01 = minv[1][0] * minv[3][1] - minv[1][1] * minv[3][0];
	const f32 det2_13_02 = minv[1][0] * minv[3][2] - minv[1][2] * minv[3][0];
	const f32 det2_13_03 = minv[1][0] * minv[3][3] - minv[1][3] * minv[3][0];
	const f32 det2_13_12 = minv[1][1] * minv[3][2] - minv[1][2] * minv[3][1];
	const f32 det2_13_13 = minv[1][1] * minv[3][3] - minv[1][3] * minv[3][1];
	const f32 det2_13_23 = minv[1][2] * minv[3][3] - minv[1][3] * minv[3][2];

	// remaining 3x3 sub-determinants
	const f32 det3_203_012 = minv[2][0] * det2_03_12 - minv[2][1] * det2_03_02 + minv[2][2] * det2_03_01;
	const f32 det3_203_013 = minv[2][0] * det2_03_13 - minv[2][1] * det2_03_03 + minv[2][3] * det2_03_01;
	const f32 det3_203_023 = minv[2][0] * det2_03_23 - minv[2][2] * det2_03_03 + minv[2][3] * det2_03_02;
	const f32 det3_203_123 = minv[2][1] * det2_03_23 - minv[2][2] * det2_03_13 + minv[2][3] * det2_03_12;

	const f32 det3_213_012 = minv[2][0] * det2_13_12 - minv[2][1] * det2_13_02 + minv[2][2] * det2_13_01;
	const f32 det3_213_013 = minv[2][0] * det2_13_13 - minv[2][1] * det2_13_03 + minv[2][3] * det2_13_01;
	const f32 det3_213_023 = minv[2][0] * det2_13_23 - minv[2][2] * det2_13_03 + minv[2][3] * det2_13_02;
	const f32 det3_213_123 = minv[2][1] * det2_13_23 - minv[2][2] * det2_13_13 + minv[2][3] * det2_13_12;

	const f32 det3_301_012 = minv[3][0] * det2_01_12 - minv[3][1] * det2_01_02 + minv[3][2] * det2_01_01;
	const f32 det3_301_013 = minv[3][0] * det2_01_13 - minv[3][1] * det2_01_03 + minv[3][3] * det2_01_01;
	const f32 det3_301_023 = minv[3][0] * det2_01_23 - minv[3][2] * det2_01_03 + minv[3][3] * det2_01_02;
	const f32 det3_301_123 = minv[3][1] * det2_01_23 - minv[3][2] * det2_01_13 + minv[3][3] * det2_01_12;

	minv[0][0] = - det3_213_123 * det_inv;
	minv[1][0] = + det3_213_023 * det_inv;
	minv[2][0] = - det3_213_013 * det_inv;
	minv[3][0] = + det3_213_012 * det_inv;

	minv[0][1] = + det3_203_123 * det_inv;
	minv[1][1] = - det3_203_023 * det_inv;
	minv[2][1] = + det3_203_013 * det_inv;
	minv[3][1] = - det3_203_012 * det_inv;

	minv[0][2] = + det3_301_123 * det_inv;
	minv[1][2] = - det3_301_023 * det_inv;
	minv[2][2] = + det3_301_013 * det_inv;
	minv[3][2] = - det3_301_012 * det_inv;

	minv[0][3] = - det3_201_123 * det_inv;
	minv[1][3] = + det3_201_023 * det_inv;
	minv[2][3] = - det3_201_013 * det_inv;
	minv[3][3] = + det3_201_012 * det_inv;

	return minv;
}

// quat

quat operator-(const quat &a)                { return quat(-a.x, -a.y, -a.z, -a.w); }
quat operator+(const quat &a, const quat &b) { return quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
quat operator-(const quat &a, const quat &b) { return quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }

quat operator*(const quat &a, const quat &b) {
	return quat(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
                a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

vec3 operator*(const quat &a, const vec3 &b) {
	const f32 xxzz = a.x * a.x - a.z * a.z;
	const f32 wwyy = a.w * a.w - a.y * a.y;

	const f32 xw2 = a.x * a.w * 2.0f;
	const f32 xy2 = a.x * a.y * 2.0f;
	const f32 xz2 = a.x * a.z * 2.0f;
	const f32 yw2 = a.y * a.w * 2.0f;
	const f32 yz2 = a.y * a.z * 2.0f;
	const f32 zw2 = a.z * a.w * 2.0f;

    const f32 xx = a.x * a.x;
    const f32 yy = a.y * a.y;
    const f32 zz = a.z * a.z;
    const f32 ww = a.w * a.w;
    
	return vec3((xxzz + wwyy) * b.x + (xy2 + zw2) * b.y	        + (xz2 - yw2)   * b.z,
                (xy2 - zw2)   * b.x + (yy + ww - xx - zz) * b.y + (yz2 + xw2)   * b.z,
                (xz2 + yw2)   * b.x + (yz2 - xw2) * b.y         + (wwyy - xxzz) * b.z);
}

quat operator*(const quat &a, f32 b) { return quat(a.x * b, a.y * b, a.z * b, a.w * b); }

quat &operator+=(quat &a, const quat &b) { a = a + b; return a; }
quat &operator-=(quat &a, const quat &b) { a = a - b; return a; }
quat &operator*=(quat &a, const quat &b) { a = a * b; return a; } 
quat &operator*=(quat &a, f32 b)         { a = a * b; return a; }

bool operator==(const quat &a, const quat &b) { return equal(a, b); }
bool operator!=(const quat &a, const quat &b) { return !(a == b); }

quat operator*(const f32 a, const quat &b)   { return b * a; }
vec3 operator*(const vec3 &a, const quat &b) { return b * a; }

bool equal(const quat &a, const quat &b, const f32 epsilon) {
	return Abs(a.x - b.x) <= epsilon &&
		   Abs(a.y - b.y) <= epsilon &&
		   Abs(a.z - b.z) <= epsilon &&
		   Abs(a.w - b.w) <= epsilon;
}

quat inverse(const quat &a) { return quat(-a.x, -a.y, -a.z, a.w); }
f32  length(const quat &a)  { return Sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w); }

quat normalize(const quat &a) {
    quat r = a;
	const f32 l = length(a);
	if (l != 0.0f) {
		const f32 lr = 1 / l;
		r.x *= lr; r.y *= lr; r.z *= lr; r.w *= lr;
	}
	return r;
}

vec3 forward(const quat &a) {
    vec3 v;
    v.x = 0 + 2 * (a.x * a.z + a.w * a.y);
    v.y = 0 + 2 * (a.y * a.z - a.w * a.x);
    v.z = 1 - 2 * (a.x * a.x + a.y * a.y);
    return normalize(v);
}

f32 calc_w(const quat &a) { return Sqrt(Abs(1.0f - (a.x * a.x + a.y * a.y + a.z * a.z))); }

const char *to_string(const quat &q) {
    static char buffers[4][32];
    static s32 buffer_index = 0;
    buffer_index = (buffer_index + 1) % 4;

    char* buffer = buffers[buffer_index];
    stbsp_snprintf(buffer, 32, "(%.3f %.3f %.3f %.3f)", q.x, q.y, q.z, q.w);
    return buffer;
}

quat quat_from_axis_angle(const vec3 &axes, f32 deg) {
    const f32 angle  = Rad(deg * 0.5f);
    const f32 factor = Sin(angle);

    const f32 x = axes.x * factor;
    const f32 y = axes.y * factor;
    const f32 z = axes.z * factor;

    const f32 w = Cos(angle);

    return normalize(quat(x, y, z, w));
}
