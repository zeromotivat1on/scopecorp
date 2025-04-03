#include "pch.h"
#include "math/quat.h"
#include "math/matrix.h"
#include "math/math_core.h"

#include "stb_sprintf.h"

quat::quat()
	: x(0.0f), y(0.0f), z(0.0f), w(1.0f) {
}

quat::quat(f32 a)
	: x(a), y(a), z(a), w(1.0f) {
}

quat::quat(f32 a, f32 b, f32 c, f32 d)
	: x(a), y(b), z(c), w(d) {
}

f32 quat::operator[](s32 index) const {
	return (&x)[index];
}

f32 &quat::operator[](s32 index) {
	return (&x)[index];
}

quat quat::operator-() const {
	return quat(-x, -y, -z, -w);
}

quat quat::operator+(const quat &a) const {
	return quat(x + a.x, y + a.y, z + a.z, w + a.w);
}

quat quat::operator-(const quat &a) const {
	return quat(x - a.x, y - a.y, z - a.z, w - a.w);
}

quat quat::operator*(const quat &a) const {
	return quat(w * a.x + x * a.w + y * a.z - z * a.y,
                w * a.y + y * a.w + z * a.x - x * a.z,
                w * a.z + z * a.w + x * a.y - y * a.x,
                w * a.w - x * a.x - y * a.y - z * a.z);
}

vec3 quat::operator*(const vec3 &a) const {
	const f32 xxzz = x * x - z * z;
	const f32 wwyy = w * w - y * y;

	const f32 xw2 = x * w * 2.0f;
	const f32 xy2 = x * y * 2.0f;
	const f32 xz2 = x * z * 2.0f;
	const f32 yw2 = y * w * 2.0f;
	const f32 yz2 = y * z * 2.0f;
	const f32 zw2 = z * w * 2.0f;

	return vec3((xxzz + wwyy) * a.x + (xy2 + zw2) * a.y	+ (xz2 - yw2) * a.z,
                (xy2 - zw2) * a.x + (y*y + w*w - x*x - z*z) * a.y + (yz2 + xw2) * a.z,
                (xz2 + yw2) * a.x + (yz2 - xw2) * a.y + (wwyy - xxzz) * a.z);
}

quat quat::operator*(f32 a) const {
	return quat(x * a, y * a, z * a, w * a);
}

quat &quat::operator+=(const quat &a) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;
	return *this;
}

quat &quat::operator-=(const quat &a) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;
	return *this;
}

quat &quat::operator*=(const quat &a) {
	*this = *this * a;
	return *this;
}

quat &quat::operator*=(f32 a) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;
	return *this;
}

bool quat::operator==(const quat &a) const {
	return equal(a);
}

bool quat::operator!=(const quat &a) const {
	return !(*this == a);
}

quat operator*(const f32 a, const quat &b) {
	return b * a;
}

vec3 operator*(const vec3 &a, const quat &b) {
	return b * a;
}

quat &quat::set(f32 a, f32 b, f32 c, f32 d) {
	x = a;
	y = b;
	z = c;
	w = d;
	return *this;
}

bool quat::equal(const quat &a) const {
	return (x == a.x) && (y == a.y) && (z == a.z) && (w == a.w);
}

bool quat::equal(const quat &a, const f32 epsilon) const {
	return absf(x - a.x) <= epsilon &&
		   absf(y - a.y) <= epsilon &&
		   absf(z - a.z) <= epsilon &&
		   absf(w - a.w) <= epsilon;
}

quat quat::inverse() const {
	return quat(-x, -y, -z, w);
}

f32 quat::length() const {
	return sqrt(x * x + y * y + z * z + w * w);
}

quat &quat::normalize() {
	const f32 len = length();
	if (len != 0.0f) {
		const f32 length_inv = 1 / len;
		x *= length_inv;
		y *= length_inv;
		z *= length_inv;
		w *= length_inv;
	}
	return *this;
}

f32 quat::calc_w() const {
	return sqrt(absf(1.0f - (x * x + y * y + z * z)));
}


mat3 quat::to_mat3() const {
    mat3 mat;

    const f32 x2 = x + x;
    const f32 y2 = y + y;
    const f32 z2 = z + z;

    const f32 xx = x * x2;
    const f32 xy = x * y2;
    const f32 xz = x * z2;

    const f32 yy = y * y2;
    const f32 yz = y * z2;
    const f32 zz = z * z2;

    const f32 wx = w * x2;
    const f32 wy = w * y2;
    const f32 wz = w * z2;

    mat[0][0] = 1.0f - (yy + zz);
    mat[1][0] = xy - wz;
    mat[2][0] = xz + wy;

    mat[0][1] = xy + wz;
    mat[1][1] = 1.0f - (xx + zz);
    mat[2][1] = yz - wx;

    mat[0][2] = xz - wy;
    mat[1][2] = yz + wx;
    mat[2][2] = 1.0f - (xx + yy);

    return mat;
}

mat4 quat::to_mat4() const {
    return to_mat3().to_mat4();
}

s32 quat::dimension() const {
	return 4;
}

const f32 *quat::ptr() const {
	return &x;
}

f32 *quat::ptr() {
	return &x;
}

// Ex

const char* to_string(const quat &q) {
    static char buffers[4][32];
    static s32 buffer_index = 0;
    buffer_index = (buffer_index + 1) % 4;

    char* buffer = buffers[buffer_index];
    stbsp_snprintf(buffer, 32, "(%.3f %.3f %.3f %.3f)", q.x, q.y, q.z, q.w);
    return buffer;
}

quat quat_from_axis_angle(const vec3 &axes, f32 deg) {
    const f32 angle  = rad(deg * 0.5f);
    const f32 factor = sin(angle);

    const f32 x = axes.x * factor;
    const f32 y = axes.y * factor;
    const f32 z = axes.z * factor;

    const f32 w = cos(angle);

    return quat(x, y, z, w).normalize();
}
