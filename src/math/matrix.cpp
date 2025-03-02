#include "pch.h"
#include "math/matrix.h"
#include "math/quat.h"
#include "math/math_core.h"
#include <string.h>

// Matrix2

mat2::mat2() {
}

mat2::mat2(const vec2& x, const vec2& y) {
	mat[0] = x;
	mat[1] = y;
}

mat2::mat2(f32 xx, f32 xy, f32 yx, f32 yy) {
	mat[0][0] = xx; mat[0][1] = xy;
	mat[1][0] = yx; mat[1][1] = yy;
}

mat2::mat2(const f32 src[2][2]) {
	memcpy(mat, src, 2 * 2 * sizeof(f32));
}

const vec2& mat2::operator[](s32 index) const {
	return mat[index];
}

vec2& mat2::operator[](s32 index) {
	return mat[index];
}

mat2 mat2::operator-() const {
	return mat2(-mat[0][0], -mat[0][1],
                -mat[1][0], -mat[1][1]);
}

mat2 mat2::operator+(const mat2& a) const {
	return mat2(mat[0][0] + a[0][0], mat[0][1] + a[0][1],
                mat[1][0] + a[1][0], mat[1][1] + a[1][1]);
}

mat2 mat2::operator-(const mat2& a) const {
	return mat2(mat[0][0] - a[0][0], mat[0][1] - a[0][1],
                mat[1][0] - a[1][0], mat[1][1] - a[1][1]);
}

mat2 mat2::operator*(const mat2& a) const {
	return mat2(mat[0][0] * a[0][0] + mat[0][1] * a[1][0],
                mat[0][0] * a[0][1] + mat[0][1] * a[1][1],
                mat[1][0] * a[0][0] + mat[1][1] * a[1][0],
                mat[1][0] * a[0][1] + mat[1][1] * a[1][1]);
}

vec2 mat2::operator*(const vec2& vec) const {
	return vec2(mat[0][0] * vec[0] + mat[0][1] * vec[1],
                mat[1][0] * vec[0] + mat[1][1] * vec[1]);
}

mat2 mat2::operator*(f32 a) const {
	return mat2(mat[0][0] * a, mat[0][1] * a,
                mat[1][0] * a, mat[1][1] * a);
}

mat2& mat2::operator+=(const mat2& a) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1];
	return *this;
}

mat2& mat2::operator-=(const mat2& a) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1];
	return *this;
}

mat2& mat2::operator*=(const mat2& a) {
	mat[0][0] = mat[0][0] * a[0][0] + mat[0][1] * a[1][0];
	mat[0][1] = mat[0][0] * a[0][1] + mat[0][1] * a[1][1];
	mat[1][0] = mat[1][0] * a[0][0] + mat[1][1] * a[1][0];
	mat[1][1] = mat[1][0] * a[0][1] + mat[1][1] * a[1][1];
	return *this;
}

mat2& mat2::operator*=(f32 a) {
	mat[0][0] *= a; mat[0][1] *= a;
	mat[1][0] *= a; mat[1][1] *= a;
	return *this;
}

bool mat2::operator==(const mat2& a) const {
	return equal(a);
}

bool mat2::operator!=(const mat2& a) const {
	return !(*this == a);
}

vec2 operator*(const vec2& vec, const mat2& mat) {
	return mat * vec;
}

mat2 operator*(f32 a, const mat2& mat) {
	return mat * a;
}

vec2& operator*=(vec2& vec, const mat2& mat) {
	vec = mat * vec;
	return vec;
}

bool mat2::equal(const mat2& a) const {
	return mat[0].equal(a[0]) && mat[1].equal(a[1]);
}

bool mat2::equal(const mat2& a, f32 epsilon) const {
	return mat[0].equal(a[0], epsilon) && mat[1].equal(a[1], epsilon);
}

mat2& mat2::zero() {
	memset(mat, 0, sizeof(mat2));
	return *this;
}

mat2& mat2::identity() {
	*this = mat2_identity();
	return *this;
}

bool mat2::identity(f32 epsilon) const {
	return equal(mat2_identity(), epsilon);
}

bool mat2::symmetric(f32 epsilon) const {
	return absf(mat[0][1] - mat[1][0]) < epsilon;
}

bool mat2::diagonal(f32 epsilon) const {
	return absf(mat[0][1]) <= epsilon &&
		   absf(mat[1][0]) <= epsilon;
}

f32 mat2::trace() const {
	return mat[0][0] + mat[1][1];
}

mat2 mat2::transpose_copy() const {
	return mat2(mat[0][0], mat[1][0],
                mat[0][1], mat[1][1]);
}

mat2& mat2::transpose() {
	swap(mat[0][1], mat[1][0]);
	return *this;
}

s32 mat2::dimension() const {
	return 4;
}

const f32* mat2::ptr() const {
	return mat[0].ptr();
}

f32* mat2::ptr() {
	return mat[0].ptr();
}

// Matrix3

mat3::mat3() {
}

mat3::mat3(const vec3& x, const vec3& y, const vec3& z) {
	mat[0] = x;
	mat[1] = y;
	mat[2] = z;
}

mat3::mat3(f32 xx, f32 xy, f32 xz, f32 yx, f32 yy, f32 yz, f32 zx, f32 zy, f32 zz) {
	mat[0][0] = xx; mat[0][1] = xy; mat[0][2] = xz;
	mat[1][0] = yx; mat[1][1] = yy; mat[1][2] = yz;
	mat[2][0] = zx; mat[2][1] = zy; mat[2][2] = zz;
}

mat3::mat3(const f32 src[3][3]) {
	memcpy(mat, src, 3 * 3 * sizeof(f32));
}

const vec3& mat3::operator[](s32 index) const {
	return mat[index];
}

vec3& mat3::operator[](s32 index) {
	return mat[index];
}

mat3 mat3::operator-() const {
	return mat3(-mat[0][0], -mat[0][1], -mat[0][2],
                -mat[1][0], -mat[1][1], -mat[1][2],
                -mat[2][0], -mat[2][1], -mat[2][2]);
}

mat3 mat3::operator+(const mat3& a) const {
	return mat3(mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2],
                mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2],
                mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2]);
}

mat3 mat3::operator-(const mat3& a) const {
	return mat3(mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2],
                mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2],
                mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2]);
}

mat3 mat3::operator*(const mat3& a) const {
	mat3 dst;
	f32* dstPtr		= reinterpret_cast<f32*>(&dst);
	const f32* ptr	= reinterpret_cast<const f32*>(this);
	const f32* aptr = reinterpret_cast<const f32*>(&a);

	for (s32 i = 0; i < 3; ++i) {
		for (s32 j = 0; j < 3; ++j) {
			*dstPtr = ptr[0] * aptr[0 * 3 + j] +
					  ptr[1] * aptr[1 * 3 + j] +
					  ptr[2] * aptr[2 * 3 + j];
			dstPtr++;
		}
		ptr += 3;
	}

	return dst;
}

vec3 mat3::operator*(const vec3& vec) const {
	return vec3(mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2],
                mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2],
                mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2]);
}

mat3 mat3::operator*(f32 a) const {
	return mat3(mat[0][0] * a, mat[0][1] * a, mat[0][2] * a,
                mat[1][0] * a, mat[1][1] * a, mat[1][2] * a,
                mat[2][0] * a, mat[2][1] * a, mat[2][2] * a);
}

mat3& mat3::operator+=(const mat3& a) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1]; mat[0][2] += a[0][2];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1]; mat[1][2] += a[1][2];
	mat[2][0] += a[2][0]; mat[2][1] += a[2][1]; mat[2][2] += a[2][2];
	return *this;
}

mat3& mat3::operator-=(const mat3& a) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1]; mat[0][2] -= a[0][2];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1]; mat[1][2] -= a[1][2];
	mat[2][0] -= a[2][0]; mat[2][1] -= a[2][1]; mat[2][2] -= a[2][2];
	return *this;
}

mat3& mat3::operator*=(const mat3& a) {
	*this = *this * a;
	return *this;
}

mat3& mat3::operator*=(f32 a) {
	mat[0][0] *= a; mat[0][1] *= a; mat[0][2] *= a;
	mat[1][0] *= a; mat[1][1] *= a; mat[1][2] *= a;
	mat[2][0] *= a; mat[2][1] *= a; mat[2][2] *= a;
	return *this;
}

bool mat3::operator==(const mat3& a) const {
	return equal(a);
}

bool mat3::operator!=(const mat3& a) const {
	return !(*this == a);
}

vec3 operator*(const vec3& vec, const mat3& mat) {
	return mat * vec;
}

mat3 operator*(f32 a, const mat3& mat) {
	return mat * a;
}

vec3& operator*=(vec3& vec, const mat3& mat) {
	vec = mat * vec;
	return vec;
}

bool mat3::equal(const mat3& a) const {
	return mat[0].equal(a[0]) &&
		   mat[1].equal(a[1]) &&
		   mat[2].equal(a[2]);
}

bool mat3::equal(const mat3& a, f32 epsilon) const {
	return mat[0].equal(a[0], epsilon) &&
		   mat[1].equal(a[1], epsilon) &&
		   mat[2].equal(a[2], epsilon);
}

mat3& mat3::zero() {
	memset(mat, 0, sizeof(mat3));
	return *this;
}

mat3& mat3::identity() {
	*this = mat3_identity();
	return *this;
}

bool mat3::identity(f32 epsilon) const {
	return equal(mat3_identity(), epsilon);
}

bool mat3::symmetric(f32 epsilon) const {
	return absf(mat[0][1] - mat[1][0]) <= epsilon &&
	   	   absf(mat[0][2] - mat[2][0]) <= epsilon &&
	   	   absf(mat[1][2] - mat[2][1]) <= epsilon;
}

bool mat3::diagonal(f32 epsilon) const {
	return absf(mat[0][1]) <= epsilon &&
		   absf(mat[0][2]) <= epsilon &&
		   absf(mat[1][0]) <= epsilon &&
		   absf(mat[1][2]) <= epsilon &&
		   absf(mat[2][0]) <= epsilon &&
		   absf(mat[2][1]) <= epsilon;
}

bool mat3::rotated() const {
	return mat[0][1] != 0.0f || mat[0][2] != 0.0f ||
	   	   mat[1][0] != 0.0f || mat[1][2] != 0.0f ||
	   	   mat[2][0] != 0.0f || mat[2][1] != 0.0f;
}

f32 mat3::trace() const {
	return mat[0][0] + mat[1][1] + mat[2][2];
}

mat3 mat3::transpose_copy() const {
	return mat3(mat[0][0], mat[1][0], mat[2][0],
                mat[0][1], mat[1][1], mat[2][1],
                mat[0][2], mat[1][2], mat[2][2]);
}

mat3& mat3::transpose() {
	swap(mat[0][1], mat[1][0]);
	swap(mat[0][2], mat[2][0]);
	swap(mat[1][2], mat[2][1]);
	return *this;
}

s32 mat3::dimension() const {
	return 9;
}

mat4 mat3::to_mat4() const {
	return mat4(mat[0][0], mat[0][1], mat[0][2], 0.0f,
                mat[1][0], mat[1][1], mat[1][2], 0.0f,
                mat[2][0], mat[2][1], mat[2][2], 0.0f,
                0.0f,	   0.0f,	  0.0f,		 1.0f);
}

const f32* mat3::ptr() const {
	return mat[0].ptr();
}

f32* mat3::ptr() {
	return mat[0].ptr();
}

// Matrix4

mat4::mat4() {
}

mat4::mat4(const vec4& x, const vec4& y, const vec4& z, const vec4& w) {
	mat[0] = x;
	mat[1] = y;
	mat[2] = z;
	mat[3] = w;
}

mat4::mat4(f32 xx, f32 xy, f32 xz, f32 xw,
           f32 yx, f32 yy, f32 yz, f32 yw,
           f32 zx, f32 zy, f32 zz, f32 zw,
           f32 wx, f32 wy, f32 wz, f32 ww) {
	mat[0][0] = xx; mat[0][1] = xy; mat[0][2] = xz; mat[0][3] = xw;
	mat[1][0] = yx; mat[1][1] = yy; mat[1][2] = yz; mat[1][3] = yw;
	mat[2][0] = zx; mat[2][1] = zy; mat[2][2] = zz; mat[2][3] = zw;
	mat[3][0] = wx; mat[3][1] = wy; mat[3][2] = wz; mat[3][3] = ww;
}

mat4::mat4(const mat3& rotation, const vec3& translation) {
	mat[0][0] = rotation[0][0];
	mat[0][1] = rotation[0][1];
	mat[0][2] = rotation[0][2];
	mat[0][3] = translation[0];

	mat[1][0] = rotation[1][0];
	mat[1][1] = rotation[1][1];
	mat[1][2] = rotation[1][2];
	mat[1][3] = translation[1];

	mat[2][0] = rotation[2][0];
	mat[2][1] = rotation[2][1];
	mat[2][2] = rotation[2][2];
	mat[2][3] = translation[2];

	mat[3][0] = 0.0f;
	mat[3][1] = 0.0f;
	mat[3][2] = 0.0f;
	mat[3][3] = 1.0f;
}

mat4::mat4(const f32 src[4][4]) {
	memcpy(mat, src, 4 * 4 * sizeof(f32));
}

mat4::mat4(const f32* src) {
	memcpy(mat, src, 4 * 4 * sizeof(f32));
}

const vec4& mat4::operator[](s32 index) const {
	return mat[index];
}

vec4& mat4::operator[](s32 index) {
	return mat[index];
}

mat4 mat4::operator+(const mat4& a) const {
	return mat4(mat[0][0] + a[0][0], mat[0][1] + a[0][1], mat[0][2] + a[0][2], mat[0][3] + a[0][3],
                mat[1][0] + a[1][0], mat[1][1] + a[1][1], mat[1][2] + a[1][2], mat[1][3] + a[1][3],
                mat[2][0] + a[2][0], mat[2][1] + a[2][1], mat[2][2] + a[2][2], mat[2][3] + a[2][3],
                mat[3][0] + a[3][0], mat[3][1] + a[3][1], mat[3][2] + a[3][2], mat[3][3] + a[3][3]);
}

mat4 mat4::operator-(const mat4& a) const {
	return mat4(mat[0][0] - a[0][0], mat[0][1] - a[0][1], mat[0][2] - a[0][2], mat[0][3] - a[0][3],
                mat[1][0] - a[1][0], mat[1][1] - a[1][1], mat[1][2] - a[1][2], mat[1][3] - a[1][3],
                mat[2][0] - a[2][0], mat[2][1] - a[2][1], mat[2][2] - a[2][2], mat[2][3] - a[2][3],
                mat[3][0] - a[3][0], mat[3][1] - a[3][1], mat[3][2] - a[3][2], mat[3][3] - a[3][3]);
}

mat4 mat4::operator*(const mat4& a) const {
	mat4 dst;
	f32* dstPtr		= reinterpret_cast<f32*>(&dst);
	const f32* ptr	= reinterpret_cast<const f32*>(this);
	const f32* aptr = reinterpret_cast<const f32*>(&a);

	for (s32 i = 0; i < 4; ++i) {
		for (s32 j = 0; j < 4; ++j) {
			*dstPtr = ptr[0] * aptr[0 * 4 + j] +
					  ptr[1] * aptr[1 * 4 + j] +
					  ptr[2] * aptr[2 * 4 + j] +
					  ptr[3] * aptr[3 * 4 + j];
			dstPtr++;
		}
		ptr += 4;
	}

	return dst;
}

vec4 mat4::operator*(const vec4& vec) const {
	return vec4(mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3],
                mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3],
                mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3],
                mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3] * vec[3]);
}

vec3 mat4::operator*(const vec3& vec) const {
	const f32 scalar = mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[3][3];

	if (scalar == 0.0f) return vec3(0.0f, 0.0f, 0.0f);

	if (scalar == 1.0f) {
		return vec3(mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3],
                    mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3],
                    mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3]);
	}

	const f32 inv_scalar = 1.0f / scalar;
    return vec3((mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3]) * inv_scalar,
                (mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3]) * inv_scalar,
                (mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3]) * inv_scalar);
}

mat4 mat4::operator*(f32 a) const {
	return mat4(mat[0][0] * a, mat[0][1] * a, mat[0][2] * a, mat[0][3] * a,
                mat[1][0] * a, mat[1][1] * a, mat[1][2] * a, mat[1][3] * a,
                mat[2][0] * a, mat[2][1] * a, mat[2][2] * a, mat[2][3] * a,
                mat[3][0] * a, mat[3][1] * a, mat[3][2] * a, mat[3][3] * a);
}

mat4& mat4::operator+=(const mat4& a) {
	mat[0][0] += a[0][0]; mat[0][1] += a[0][1]; mat[0][2] += a[0][2]; mat[0][3] += a[0][3];
	mat[1][0] += a[1][0]; mat[1][1] += a[1][1]; mat[1][2] += a[1][2]; mat[1][3] += a[1][3];
	mat[2][0] += a[2][0]; mat[2][1] += a[2][1]; mat[2][2] += a[2][2]; mat[2][3] += a[2][3];
	mat[3][0] += a[3][0]; mat[3][1] += a[3][1]; mat[3][2] += a[3][2]; mat[3][3] += a[3][3];
	return *this;
}

mat4& mat4::operator-=(const mat4& a) {
	mat[0][0] -= a[0][0]; mat[0][1] -= a[0][1]; mat[0][2] -= a[0][2]; mat[0][3] -= a[0][3];
	mat[1][0] -= a[1][0]; mat[1][1] -= a[1][1]; mat[1][2] -= a[1][2]; mat[1][3] -= a[1][3];
	mat[2][0] -= a[2][0]; mat[2][1] -= a[2][1]; mat[2][2] -= a[2][2]; mat[2][3] -= a[2][3];
	mat[3][0] -= a[3][0]; mat[3][1] -= a[3][1]; mat[3][2] -= a[3][2]; mat[3][3] -= a[3][3];
	return *this;
}

mat4& mat4::operator*=(const mat4& a) {
	*this = *this * a;
	return *this;
}

mat4& mat4::operator*=(f32 a) {
	mat[0][0] *= a; mat[0][1] *= a; mat[0][2] *= a; mat[0][3] *= a;
	mat[1][0] *= a; mat[1][1] *= a; mat[1][2] *= a; mat[1][3] *= a;
	mat[2][0] *= a; mat[2][1] *= a; mat[2][2] *= a; mat[2][3] *= a;
	mat[3][0] *= a; mat[3][1] *= a; mat[3][2] *= a; mat[3][3] *= a;
    return *this;
}

bool mat4::operator==(const mat4& a) const {
	return equal(a);
}

bool mat4::operator!=(const mat4& a) const {
	return !(*this == a);
}

vec4 operator*(const vec4& vec, const mat4& mat) {
	return mat * vec;
}

vec3 operator*(const vec3& vec, const mat4& mat) {
	return mat * vec;
}

mat4 operator*(f32 a, const mat4& mat) {
	return mat * a;
}

vec4& operator*=(vec4& vec, const mat4& mat) {
	vec = mat * vec;
	return vec;
}

vec3& operator*=(vec3& vec, const mat4& mat) {
	vec = mat * vec;
	return vec;
}

bool mat4::equal(const mat4& a) const {
	return mat[0].equal(a[0]) &&
		   mat[1].equal(a[1]) &&
		   mat[2].equal(a[2]) &&
		   mat[3].equal(a[3]);
}

bool mat4::equal(const mat4& a, f32 epsilon) const {
	return mat[0].equal(a[0], epsilon) &&
	       mat[1].equal(a[1], epsilon) &&
	       mat[2].equal(a[2], epsilon) &&
	       mat[3].equal(a[3], epsilon);
}

mat4& mat4::zero() {
	memset(mat, 0, sizeof(mat4));
	return *this;
}

mat4& mat4::identity() {
	*this = mat4_identity();
	return *this;
}

bool mat4::identity(f32 epsilon) const {
	return equal(mat4_identity(), epsilon);
}

bool mat4::symmetric(f32 epsilon) const {
	for (s32 i = 1; i < 4; ++i)
		for (s32 j = 0; j < i; ++j)
			if (absf(mat[i][j] - mat[j][i]) > epsilon) return false;
	return true;
}

bool mat4::diagonal(f32 epsilon) const {
	for (s32 i = 0; i < 4; ++i)
		for (s32 j = 0; j < 4; ++j) 
			if (i != j && absf(mat[i][j]) > epsilon)  return false;
	return true;
}

bool mat4::rotated() const {
	return mat[0][1] != 0.0f || mat[0][2] != 0.0f ||
		   mat[1][0] != 0.0f || mat[1][2] != 0.0f ||
		   mat[2][0] != 0.0f || mat[2][1] != 0.0f;
}

f32 mat4::trace() const {
	return mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3];
}

mat4 mat4::transpose_copy() const {
	mat4 transpose;

	for(s32 i = 0; i < 4; ++i)
		for(s32 j = 0; j < 4; ++j)
			transpose[i][j] = mat[j][i];

	return transpose;
}

mat4& mat4::transpose() {
	for(s32 i = 0; i < 4; ++i)
		for(s32 j = i + 1; j < 4; ++j)
			swap(mat[i][j], mat[j][i]);

	return *this;
}

mat4& mat4::translate(const vec3& translation) {
	mat[3][0] = translation.x;
	mat[3][1] = translation.y;
	mat[3][2] = translation.z;
	return *this;
}

mat4& mat4::rotate(const mat4& rotation) {
	*this = *this * rotation;
	return *this;
}

mat4& mat4::rotate(const quat& rotation) {
    *this = *this * rotation.to_mat4();
    return *this;
}

mat4& mat4::scale(const vec3& scale) {
	mat[0][0] = scale.x;
	mat[1][1] = scale.y;
	mat[2][2] = scale.z;
	return *this;
}

s32 mat4::dimension() const {
	return 16;
}

const f32 *mat4::ptr() const {
	return mat[0].ptr();
}

f32 *mat4::ptr() {
	return mat[0].ptr();
}

// Ex

mat4 mat4_identity() {
	return mat4(vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
}

mat2 mat2_identity() {
	return mat2(vec2(1.0f, 0.0f), vec2(0.0f, 1.0f));
}

mat3 mat3_identity() {
	return mat3(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
}

mat4 mat4_transform(const vec3& location, const quat& rotation, const vec3& scale) {
    return mat4_identity().scale(scale).rotate(rotation).translate(location);
}

mat4 mat4_view(const vec3& eye, const vec3& at, const vec3& up) {
	const vec3 f = (at - eye).normalize();
	const vec3 r = up.cross(f).normalize();
	const vec3 u = f.cross(r);

	mat4 result;

	result[0][0] = r.x;
	result[1][0] = r.y;
	result[2][0] = r.z;
	result[3][0] = -r.dot(eye);

	result[0][1] = u.x;
	result[1][1] = u.y;
	result[2][1] = u.z;
	result[3][1] = -u.dot(eye);

	result[0][2] = -f.x;
	result[1][2] = -f.y;
	result[2][2] = -f.z;
	result[3][2] = f.dot(eye);

	result[0][3] = 0.0f;
	result[1][3] = 0.0f;
	result[2][3] = 0.0f;
	result[3][3] = 1.0f;

	return result;
}

mat4 mat4_perspective(f32 rfovy, f32 aspect, f32 n, f32 f) {
	const f32 tan_half_fovy = tan(rfovy * 0.5f);

	mat4 result;
	result[0][0] = 1.0f / (aspect * tan_half_fovy);	// x-axis scaling
	result[1][1] = 1.0f / tan_half_fovy;			// y-axis scaling
	result[2][2] = -(f + n) / (f - n);				// z-axis scaling
	result[2][3] = -1.0f;							// rh perspective divide
	result[3][2] = -(2.0f * f * n) / (f - n);		// z-axis translation

	return result;
}

mat4 mat4_orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	mat4 result;
	result[0][0] =  2.0f / (r - l);
	result[1][1] =  2.0f / (t - b);
	result[2][2] = -2.0f / (f - n);
	result[3][0] = -(r + l) / (r - l);
	result[3][1] = -(t + b) / (t - b);
	result[3][2] = -(f + n) / (f - n);
	result[3][3] =  1.0f;
	return result;
}
