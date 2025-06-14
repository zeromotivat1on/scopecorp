#pragma once

struct vec2 {
	f32 x, y;

	vec2();
	explicit vec2(f32 a);
	explicit vec2(f32 a, f32 b);
	explicit vec2(const f32 src[2]);

	f32	  operator[](s32 index) const;
	f32  &operator[](s32 index);
	vec2  operator-() const;
	vec2  operator+(const vec2 &a) const;
	vec2  operator-(const vec2 &a) const;
	f32	  operator*(const vec2 &a) const;
	vec2  operator*(f32 a) const;
	vec2  operator/(f32 a) const;
	vec2 &operator+=(const vec2 &a);
	vec2 &operator-=(const vec2 &a);
	vec2 &operator/=(const vec2 &a);
	vec2 &operator/=(f32 a);
	vec2 &operator*=(f32 a);

	bool operator==(const vec2 &a) const;
	bool operator!=(const vec2 &a) const;

	friend vec2 operator*(f32 a, vec2 b);

	vec2 &set(f32 a, f32 b);
	vec2 &zero();

	bool equal(const vec2 &a) const;
	bool equal(const vec2 &a, f32 epsilon) const;

	f32	  length() const;
	f32	  length_sqr() const;
	f32   dot(const vec2 &a) const;
	vec2 &normalize();
	vec2 &truncate(f32 length);

	s32 dimension() const;

	f32 *ptr();
	const f32 *ptr() const;
};

struct vec3 {
	f32 x, y, z;

	vec3();
	explicit vec3(f32 a);
	explicit vec3(f32 a, f32 b, f32 c);
	explicit vec3(const f32 src[3]);

	f32	  operator[](s32 index) const;
	f32  &operator[](s32 index);
	vec3  operator-() const;
	vec3  operator+(const vec3 &a) const;
	vec3  operator-(const vec3 &a) const;
	f32	  operator*(const vec3 &a) const;
	vec3  operator*(f32 a) const;
	vec3  operator/(f32 a) const;
	vec3 &operator+=(const vec3 &a);
	vec3 &operator-=(const vec3 &a);
	vec3 &operator/=(const vec3 &a);
	vec3 &operator/=(f32 a);
	vec3 &operator*=(f32 a);

	bool operator==(const vec3 &a) const;
	bool operator!=(const vec3 &a) const;

	friend vec3 operator*(f32 a, vec3 b);

	vec3 &set(f32 a, f32 b, f32 c);
	vec3 &zero();

	bool equal(const vec3 &a) const;
	bool equal(const vec3 &a, f32 epsilon) const;

	f32	  length() const;
	f32	  length_sqr() const;
	f32   dot(const vec3 &a) const;
	vec3  cross(const vec3 &a) const;
	vec3 &normalize();
	vec3 &truncate(f32 length);

	s32	dimension() const;

	const vec2 &to_vec2() const;
	vec2 &to_vec2();

	f32 *ptr();
	const f32 *ptr() const;
};

struct vec4 {
	f32 x, y, z, w;

	vec4();
	explicit vec4(f32 a);
	explicit vec4(f32 a, f32 b, f32 c, f32 d);
	explicit vec4(const f32 src[4]);

	f32	  operator[](s32 index) const;
	f32  &operator[](s32 index);
	vec4  operator-() const;
	vec4  operator+(const vec4 &a) const;
	vec4  operator-(const vec4 &a) const;
	f32	  operator*(const vec4 &a) const;
	vec4  operator*(f32 a) const;
	vec4  operator/(f32 a) const;
	vec4 &operator+=(const vec4 &a);
	vec4 &operator-=(const vec4 &a);
	vec4 &operator/=(const vec4 &a);
	vec4 &operator/=(f32 a);
	vec4 &operator*=(f32 a);

	bool operator==(const vec4 &a) const;
	bool operator!=(const vec4 &a) const;

	friend vec4 operator*(f32 a, vec4 b);

	vec4 &set(f32 a, f32 b, f32 c, f32 d);
	vec4 &zero();

	bool equal(const vec4 &a) const;
	bool equal(const vec4 &a, f32 epsilon) const;

	f32 length() const;
	f32 length_sqr() const;
	f32 dot(const vec4 &a) const;
	vec4 &normalize();
	vec4 &truncate(f32 length);

	s32	dimension() const;

	const vec2 &to_vec2() const;
	const vec3 &to_vec3() const;
	vec2 &to_vec2();
	vec3 &to_vec3();

	f32 *ptr();
	const f32 *ptr() const;
};

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
#define vec4_purple vec4(1, 0, 1, 0)

// Uses static 2d buffer internally, for debug purposes only.
const char *to_string(const vec3 &v);

vec2 normalize(const vec2 &v);
vec3 normalize(const vec3 &v);
vec4 normalize(const vec4 &v);

vec3 vector_direction(const vec3 &start, const vec3 &end);
vec3 forward(f32 yaw, f32 pitch);
vec3 right(const vec3 &start, const vec3 &end, const vec3 &up);

vec3 abs(const vec3 &a);
