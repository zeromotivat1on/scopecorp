#pragma once

struct vec3;
struct mat3;
struct mat4;

struct quat {
	f32 x, y, z, w;

	quat();
	explicit quat(f32 a);
	explicit quat(f32 a, f32 b, f32 c, f32 d);

	f32	  operator[](s32 index) const;
	f32  &operator[](s32 index);
	quat  operator-() const;
	quat  operator+(const quat &a) const;
	quat  operator-(const quat &a) const;
	quat  operator*(const quat &a) const;
	vec3  operator*(const vec3 &a) const;
	quat  operator*(f32 a) const;
	quat &operator+=(const quat &a);
	quat &operator-=(const quat &a);
	quat &operator*=(const quat &a);
	quat &operator*=(f32 a);

	bool operator==(const quat &a) const;
	bool operator!=(const quat &a) const;

	friend quat	operator*(const f32 a, const quat &b);
	friend vec3	operator*(const vec3 &a, const quat &b);

	quat &set(f32 a, f32 b, f32 c, f32 d);

	bool equal(const quat &a) const;
	bool equal(const quat &a, const f32 epsilon) const;

	quat  inverse() const;
	f32	  length() const;
	quat &normalize();

	f32	calc_w() const;

	mat3 to_mat3() const;
	mat4 to_mat4() const;

	s32	dimension() const;

	const f32 *ptr() const;
	f32 *ptr();
};

const char* to_string(const quat &q);
quat quat_from_axis_angle(const vec3 &axes, f32 deg);

vec3 get_forward_vector(const quat &q);
