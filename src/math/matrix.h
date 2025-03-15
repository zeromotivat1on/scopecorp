#pragma once

#include "vector.h"

// Matrices follow *row-major* convention.
// Matrix operations follow *right-handed* convention.

struct quat;

inline constexpr f32 MATRIX_EPSILON	    = 1e-06f;
inline constexpr f32 MATRIX_INV_EPSILON	= 1e-14f;

struct mat2 {
    vec2 mat[2];

       		 mat2();
	explicit mat2(const vec2& x, const vec2& y);
	explicit mat2(f32 xx, f32 xy, f32 yx, f32 yy);
	explicit mat2(const f32 src[2][2]);

	const vec2&	operator[](s32 index) const;
	vec2&		operator[](s32 index);
	mat2		operator-() const;
   	mat2		operator+(const mat2& a) const;
   	mat2		operator-(const mat2& a) const;
   	mat2		operator*(const mat2& a) const;
	vec2		operator*(const vec2& vec) const;
   	mat2		operator*(f32 a) const;
   	mat2&		operator+=(const mat2& a);
   	mat2&		operator-=(const mat2& a);
	mat2&		operator*=(const mat2& a);
   	mat2&		operator*=(f32 a);

   	bool operator==(const mat2& a) const;
   	bool operator!=(const mat2& a) const;

	friend vec2	 operator*(const vec2& vec, const mat2& mat);
   	friend mat2	 operator*(f32 a, const mat2& mat);
	friend vec2& operator*=(vec2& vec, const mat2& mat);

	bool equal(const mat2& a) const;
	bool equal(const mat2& a, f32 epsilon) const;

	mat2& zero();
	mat2& identity();
	bool  identity(f32 epsilon = MATRIX_EPSILON) const;
	bool  symmetric(f32 epsilon = MATRIX_EPSILON) const;
	bool  diagonal(f32 epsilon = MATRIX_EPSILON) const;

	f32	  trace() const;
	mat2  transpose_copy() const;
	mat2& transpose();

	s32	dimension() const;

	const f32* ptr() const;
	f32* ptr();
};

struct mat3 {
	vec3 mat[3];

			 mat3();
	explicit mat3(const vec3& x, const vec3& y, const vec3& z);
	explicit mat3(f32 xx, f32 xy, f32 xz, f32 yx, f32 yy, f32 yz, f32 zx, f32 zy, f32 zz);
	explicit mat3(const f32 src[3][3]);

	const vec3& operator[](s32 index) const;
	vec3& 		operator[](s32 index);
	mat3		operator-() const;
	mat3		operator+(const mat3& a) const;
	mat3		operator-(const mat3& a) const;
	mat3		operator*(const mat3& a) const;
	vec3		operator*(const vec3& vec) const;
	mat3		operator*(f32 a) const;
	mat3& 		operator+=(const mat3& a);
	mat3& 		operator-=(const mat3& a);
	mat3& 		operator*=(const mat3& a);
	mat3& 		operator*=(f32 a);

	bool operator==(const mat3& a) const;
	bool operator!=(const mat3& a) const;

	friend mat3	 operator*(f32 a, const mat3& mat);
	friend vec3	 operator*(const vec3& vec, const mat3& mat);
	friend vec3& operator*=(vec3& vec, const mat3& mat);

	bool equal(const mat3& a) const;
	bool equal(const mat3& a, f32 epsilon) const;

	mat3& zero();
	mat3& identity();
	bool  identity(f32 epsilon = MATRIX_EPSILON) const;
	bool  symmetric(f32 epsilon = MATRIX_EPSILON) const;
	bool  diagonal(f32 epsilon = MATRIX_EPSILON) const;
	bool  rotated() const;

	f32	  trace() const;
	mat3  transpose_copy() const;
	mat3& transpose();

	s32 dimension() const;

	struct mat4	to_mat4() const;

	const f32* ptr() const;
	f32* ptr();
};

struct mat4 {
	vec4 mat[4];

			 mat4();
	explicit mat4(const vec4& x, const vec4& y, const vec4& z, const vec4& w);
	explicit mat4(f32 xx, f32 xy, f32 xz, f32 xw,
				  f32 yx, f32 yy, f32 yz, f32 yw,
				  f32 zx, f32 zy, f32 zz, f32 zw,
				  f32 wx, f32 wy, f32 wz, f32 ww);
	explicit mat4(const mat3& rotation, const vec3& translation);
	explicit mat4(const f32 src[4][4]);
	explicit mat4(const f32* src);

	const vec4& operator[](s32 index) const;
	vec4& 		operator[](s32 index);
	mat4		operator+(const mat4& a) const;
	mat4		operator-(const mat4& a) const;
	mat4		operator*(const mat4& a) const;
	vec4		operator*(const vec4& vec) const;
	vec3		operator*(const vec3& vec) const;
	mat4		operator*(f32 a) const;
	mat4& 		operator+=(const mat4& a);
	mat4& 		operator-=(const mat4& a);
	mat4& 		operator*=(const mat4& a);
	mat4& 		operator*=(f32 a);

	bool operator==(const mat4& a) const;
	bool operator!=(const mat4& a) const;

	friend vec4	 operator*(const vec4& vec, const mat4& mat);
	friend vec3	 operator*(const vec3& vec, const mat4& mat);
	friend mat4	 operator*(f32 a, const mat4& mat);
	friend vec4& operator*=(vec4& vec, const mat4& mat);
	friend vec3& operator*=(vec3& vec, const mat4& mat);

	bool equal(const mat4& a) const;
	bool equal(const mat4& a, f32 epsilon) const;

	mat4& zero();
	mat4& identity();
	bool  identity(f32 epsilon = MATRIX_EPSILON) const;
	bool  symmetric(f32 epsilon = MATRIX_EPSILON) const;
	bool  diagonal(f32 epsilon = MATRIX_EPSILON) const;
	bool  rotated() const;

	f32	  trace() const;
	mat4  transpose_copy() const;
	mat4& transpose();

	mat4& translate(const vec3& translation);
	mat4& rotate(const mat4& rotation);
	mat4& rotate(const quat& rotation);
	mat4& scale(const vec3& scale);

	s32	dimension() const;

	const f32* ptr() const;
	f32* ptr();
};

mat2 mat2_identity();
mat3 mat3_identity();
mat4 mat4_identity();

mat4 mat4_transform(const vec3& location, const quat& rotation, const vec3& scale);
mat4 mat4_view(const vec3& eye, const vec3& at, const vec3& up);
mat4 mat4_perspective(f32 rfovy, f32 aspect, f32 n, f32 f);
mat4 mat4_orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);

mat4 inverse(const mat4 &m);
