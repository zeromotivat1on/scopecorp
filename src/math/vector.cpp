#include "pch.h"
#include "math/vector.h"
#include "math/math_core.h"

#include "stb_sprintf.h"

// Vector2

vec2::vec2()
    : x(0.0f), y(0.0f) {
}

vec2::vec2(f32 a)
    : x(a), y(a) {
}

vec2::vec2(f32 a, f32 b)
    : x(a), y(b) {
}

vec2::vec2(const f32 src[2])
    : x(src[0]), y(src[1]) {
}

f32 vec2::operator[](s32 index) const {
    return (&x)[index];
}

f32 &vec2::operator[](s32 index) {
    return (&x)[index];
}

vec2 vec2::operator-() const {
    return vec2(-x, -y);
}

vec2 vec2::operator+(const vec2 &a) const {
    return vec2(x + a.x, y + a.y);
}

vec2 vec2::operator-(const vec2 &a) const {
    return vec2(x - a.x, y - a.y);
}

f32 vec2::operator*(const vec2 &a) const {
    return dot(a);
}

vec2 vec2::operator*(f32 a) const {
    return vec2(x * a, y * a);
}

vec2 vec2::operator/(f32 a) const {
    const f32 inv = 1.0f / a;
    return vec2(x * inv, y * inv);
}

vec2 &vec2::operator+=(const vec2 &a) {
    x += a.x;
    y += a.y;
    return *this;
}

vec2 &vec2::operator-=(const vec2 &a) {
    x -= a.x;
    y -= a.y;
    return *this;
}

vec2 &vec2::operator/=(const vec2 &a) {
    x /= a.x;
    y /= a.y;
    return *this;
}

vec2 &vec2::operator/=(f32 a) {
    const f32 inv = 1.0f / a;
    x *= inv;
    y *= inv;
    return *this;
}

vec2 &vec2::operator*=(f32 a) {
    x *= a;
    y *= a;
    return *this;
}

bool vec2::operator==(const vec2 &a) const {
    return equal(a);
}

bool vec2::operator!=(const vec2 &a) const {
    return !(*this == a);
}

vec2 operator*(f32 a, vec2 b) {
    return vec2(b.x * a, b.y * a);
}

vec2 &vec2::set(f32 a, f32 b) {
    x = a;
    y = b;
    return *this;
}

vec2 &vec2::zero() {
    x = y = 0.0f;
    return *this;
}

bool vec2::equal(const vec2 &a) const {
    return x == a.x && y == a.y;
}

bool vec2::equal(const vec2 &a, f32 epsilon) const {
    return absf(x - a.x) <= epsilon &&
           absf(y - a.y) <= epsilon;
}

f32 vec2::length() const {
    return sqrt(x * x + y * y);
}

f32 vec2::length_sqr() const {
    return x * x + y * y;
}

f32 vec2::dot(const vec2 &a) const {
    return x * a.x + y * a.y;
}

vec2 &vec2::normalize() {
    const f32 length_inv = sqrt_inv(x * x + y * y);

    x *= length_inv;
    y *= length_inv;

    return *this;
}

vec2 &vec2::truncate(f32 length) {
    if (length == 0.0f) {
        zero();
        return *this;
    }

    const f32 length_square = x * x + y * y;
    if (length_square > length * length) {
        const f32 scale_factor = length * sqrt_inv(length_square);
        x *= scale_factor;
        y *= scale_factor;
    }

    return *this;
}

s32 vec2::dimension() const {
    return 2;
}

f32* vec2::ptr() {
    return &x;
}

const f32* vec2::ptr() const {
    return &x;
}

// Vector3

vec3::vec3()
    : x(0.0f), y(0.0f), z(0.0f) {
}

vec3::vec3(f32 a)
    : x(a), y(a), z(a) {
}

vec3::vec3(f32 a, f32 b, f32 c)
    : x(a), y(b), z(c) {
}

vec3::vec3(const f32 src[3])
    : x(src[0]), y(src[1]), z(src[2]) {
}

f32 vec3::operator[](s32 index) const {
    return (&x)[index];
}

f32 &vec3::operator[](s32 index) {
    return (&x)[index];
}

vec3 vec3::operator-() const {
    return vec3(-x, -y, -z);
}

vec3 vec3::operator+(const vec3 &a) const {
    return vec3(x + a.x, y + a.y, z + a.z);
}

vec3 vec3::operator-(const vec3 &a) const {
    return vec3(x - a.x, y - a.y, z - a.z);
}

f32 vec3::operator*(const vec3 &a) const {
    return dot(a);
}

vec3 vec3::operator*(f32 a) const {
    return vec3(x * a, y * a, z * a);
}

vec3 vec3::operator/(f32 a) const {
    const f32 inv = 1.0f / a;
    return vec3(x * inv, y * inv, z * inv);
}

vec3 &vec3::operator+=(const vec3 &a) {
    x += a.x;
    y += a.y;
    z += a.z;
    return *this;
}

vec3 &vec3::operator-=(const vec3 &a) {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    return *this;
}

vec3 &vec3::operator/=(const vec3 &a) {
    x /= a.x;
    y /= a.y;
    z /= a.z;
    return *this;
}

vec3 &vec3::operator/=(f32 a) {
    const f32 inv = 1.0f / a;
    x *= inv;
    y *= inv;
    z *= inv;
    return *this;
}

vec3 &vec3::operator*=(f32 a) {
    x *= a;
    y *= a;
    z *= a;
    return *this;
}

bool vec3::operator==(const vec3 &a) const {
    return equal(a);
}

bool vec3::operator!=(const vec3 &a) const {
    return !(*this == a);
}

vec3 operator*(f32 a, vec3 b) {
    return vec3(b.x * a, b.y * a, b.z * a);
}

vec3 &vec3::set(f32 a, f32 b, f32 c) {
    x = a;
    y = b;
    z = c;
    return *this;
}

vec3 &vec3::zero() {
    x = y = z = 0.0f;
    return *this;
}

bool vec3::equal(const vec3 &a) const {
    return (x == a.x) && (y == a.y) && (z == a.z);
}

bool vec3::equal(const vec3 &a, f32 epsilon) const {
    return absf(x - a.x) <= epsilon &&
           absf(y - a.y) <= epsilon &&
           absf(z - a.z) <= epsilon;
}

f32 vec3::length() const {
    return sqrt(x * x + y * y + z * z);
}

f32 vec3::length_sqr() const {
    return x * x + y * y + z * z;
}

f32 vec3::dot(const vec3 &a) const {
    return x * a.x + y * a.y + z * a.z;
}

vec3 vec3::cross(const vec3 &a) const {
    return vec3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x);
}

vec3 &vec3::normalize() {
    const f32 length_inv = sqrt_inv(x * x + y * y + z * z);

    x *= length_inv;
    y *= length_inv;
    z *= length_inv;

    return *this;
}

vec3 &vec3::truncate(f32 length) {
    if (length == 0.0f) {
        zero();
        return *this;
    }

    const f32 length_square = x * x + y * y + z * z;
    if (length_square > length * length) {
        const f32 scale_factor = length * sqrt_inv(length_square);
        x *= scale_factor;
        y *= scale_factor;
        z *= scale_factor;
    }

    return *this;
}

s32 vec3::dimension() const {
    return 3;
}

const vec2 &vec3::to_vec2() const {
    return *(const vec2*)this;
}

vec2 &vec3::to_vec2() {
    return *(vec2*)this;
}

f32* vec3::ptr() {
    return &x;
}

const f32* vec3::ptr() const {
    return &x;
}

// Vector4

vec4::vec4()
    : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {
}

vec4::vec4(f32 a)
    : x(a), y(a), z(a), w(a) {

}

vec4::vec4(f32 a, f32 b, f32 c, f32 d)
    : x(a), y(b), z(c), w(d) {
}

vec4::vec4(const f32 src[4])
    : x(src[0]), y(src[1]), z(src[2]), w(src[3]) {
}

f32 vec4::operator[](s32 index) const {
    return (&x)[index];
}

f32 &vec4::operator[](s32 index) {
    return (&x)[index];
}

vec4 vec4::operator-() const {
    return vec4(-x, -y, -z, -w);
}

vec4 vec4::operator+(const vec4 &a) const {
    return vec4(x + a.x, y + a.y, z + a.z, w + a.w);
}

vec4 vec4::operator-(const vec4 &a) const {
    return vec4(x - a.x, y - a.y, z - a.z, w - a.w);
}

f32 vec4::operator*(const vec4 &a) const {
    return dot(a);
}

vec4 vec4::operator*(f32 a) const {
    return vec4(x * a, y * a, z * a, w * a);
}

vec4 vec4::operator/(f32 a) const {
    const f32 inv = 1.0f / a;
    return vec4(x * inv, y * inv, z * inv, w * inv);
}

vec4 &vec4::operator+=(const vec4 &a) {
    x += a.x;
    y += a.y;
    z += a.z;
    w += a.w;
    return *this;
}

vec4 &vec4::operator-=(const vec4 &a) {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    w -= a.w;
    return *this;
}

vec4 &vec4::operator/=(const vec4 &a) {
    x /= a.x;
    y /= a.y;
    z /= a.z;
    w /= a.w;
    return *this;
}

vec4 &vec4::operator/=(f32 a) {
    const f32 inv = 1.0f / a;
    x *= inv;
    y *= inv;
    z *= inv;
    w *= inv;
    return *this;
}

vec4 &vec4::operator*=(f32 a) {
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
}

bool vec4::operator==(const vec4 &a) const {
    return equal(a);
}

bool vec4::operator!=(const vec4 &a) const {
    return !(*this == a);
}

vec4 operator*(f32 a, vec4 b) {
    return vec4(b.x * a, b.y * a, b.z * a, b.w * a);
}

vec4 &vec4::set(f32 a, f32 b, f32 c, f32 d) {
    x = a;
    y = b;
    z = c;
    w = d;
    return *this;
}

vec4 &vec4::zero() {
    x = y = z = w = 0.0f;
    return *this;
}

bool vec4::equal(const vec4 &a) const {
    return (x == a.x) && (y == a.y) && (z == a.z) && (w == a.w);
}

bool vec4::equal(const vec4 &a, f32 epsilon) const {
    return absf(x - a.x) <= epsilon &&
           absf(y - a.y) <= epsilon &&
           absf(z - a.z) <= epsilon &&
           absf(w - a.w) <= epsilon;
}

f32 vec4::length() const {
    return sqrt(x * x + y * y + z * z + w * w);
}

f32 vec4::length_sqr() const {
    return x * x + y * y + z * z + w * w;
}

f32 vec4::dot(const vec4 &a) const {
    return x * a.x + y * a.y + z * a.z + w * a.w;
}

vec4 &vec4::normalize() {
    const f32 lengthInv = sqrt_inv(x * x + y * y + z * z + w * w);

    x *= lengthInv;
    y *= lengthInv;
    z *= lengthInv;
    w *= lengthInv;

    return *this;
}

vec4 &vec4::truncate(f32 length) {
    if (length == 0.0f) {
        zero();
        return *this;
    }

    const f32 length_square = x * x + y * y + z * z + w * w;
    if (length_square > length * length) {
        const f32 scale_factor = length * sqrt_inv(length_square);
        x *= scale_factor;
        y *= scale_factor;
        z *= scale_factor;
        w *= scale_factor;
    }

    return *this;
}

s32 vec4::dimension() const {
    return 4;
}

const vec2 &vec4::to_vec2() const {
    return *(const vec2*)this;
}

vec2 &vec4::to_vec2() {
    return *(vec2*)this;
}

const vec3 &vec4::to_vec3() const {
    return *(const vec3*)this;
}

vec3 &vec4::to_vec3() {
    return *(vec3*)this;
}

f32* vec4::ptr() {
    return &x;
}

const f32* vec4::ptr() const {
    return &x;
}

// Ex

const char* to_string(const vec3 &v) {
    static char buffers[4][32];
    static s32 buffer_index = 0;
    buffer_index = (buffer_index + 1) % 4;

    char* buffer = buffers[buffer_index];
    stbsp_snprintf(buffer, 32, "(%.3f, %.3f %.3f)", v.x, v.y, v.z);
    return buffer;
}

vec3 forward(const vec3 &start, const vec3 &end) {
    return (end - start).normalize();
}

vec3 forward(f32 yaw, f32 pitch) {
    const f32 ycos = cos(rad(yaw));
    const f32 ysin = sin(rad(yaw));
    const f32 pcos = cos(rad(pitch));
    const f32 psin = sin(rad(pitch));
    return vec3(ycos * pcos, psin, ysin * pcos).normalize();
}

vec3 right(const vec3 &start, const vec3 &end, const vec3 &up) {
    return up.cross(end - start).normalize();
}

vec3 lerp(const vec3 &a, const vec3 &b, f32 alpha) {
    if (alpha <= 0.0f) return a;
    else if (alpha >= 1.0f) return b;
    return a + alpha * (b - a);
}
