#pragma once

struct vec2
{
    f32 x;
    f32 y;

             vec2();
    explicit vec2(f32 a);
    explicit vec2(f32 a, f32 b);

    f32	  operator[](s32 index) const;
    f32&  operator[](s32 index);
    vec2  operator-() const;
    vec2  operator+(const vec2& a) const;
    vec2  operator-(const vec2& a) const;
    f32	  operator*(const vec2& a) const;
    vec2  operator*(f32 a) const;
    vec2  operator/(f32 a) const;
    vec2& operator+=(const vec2& a);
    vec2& operator-=(const vec2& a);
    vec2& operator/=(const vec2& a);
    vec2& operator/=(f32 a);
    vec2& operator*=(f32 a);

    bool operator==(const vec2& a) const;
    bool operator!=(const vec2& a) const;

    friend vec2 operator*(f32 a, vec2 b);

    vec2& set(f32 a, f32 b);
    vec2& zero();

    bool equal(const vec2& a) const;
    bool equal(const vec2& a, f32 epsilon) const;

    f32	  length() const;
    f32	  length_sqr() const;
    f32   dot(const vec2& a) const;
    vec2& normalize();
    vec2& truncate(f32 length);
    
    s32 dimension() const;

    f32* ptr();
    const f32* ptr() const;
};

struct vec3
{
    f32 x;
    f32 y;
    f32 z;

             vec3();
    explicit vec3(f32 a);
    explicit vec3(f32 a, f32 b, f32 c);

    f32	  operator[](s32 index) const;
    f32&  operator[](s32 index);
    vec3  operator-() const;
    vec3  operator+(const vec3& a) const;
    vec3  operator-(const vec3& a) const;
    f32	  operator*(const vec3& a) const;
    vec3  operator*(f32 a) const;
    vec3  operator/(f32 a) const;
    vec3& operator+=(const vec3& a);
    vec3& operator-=(const vec3& a);
    vec3& operator/=(const vec3& a);
    vec3& operator/=(f32 a);
    vec3& operator*=(f32 a);

    bool operator==(const vec3& a) const;
    bool operator!=(const vec3& a) const;

    friend vec3 operator*(f32 a, vec3 b);

    vec3& set(f32 a, f32 b, f32 c);
    vec3& zero();

    bool equal(const vec3& a) const;
    bool equal(const vec3& a, f32 epsilon) const;

    f32	  length() const;
    f32	  length_sqr() const;
    f32   dot(const vec3& a) const;
    vec3  cross(const vec3& a) const;
    vec3& normalize();
    vec3& truncate(f32 length);
    
    s32	dimension() const;

    const vec2& to_vec2() const;
    vec2& to_vec2();

    f32* ptr();
    const f32* ptr() const;
};

struct vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;

             vec4();
    explicit vec4(f32 a);
    explicit vec4(f32 a, f32 b, f32 c, f32 d);

    f32	  operator[](s32 index) const;
    f32&  operator[](s32 index);
    vec4  operator-() const;
    vec4  operator+(const vec4& a) const;
    vec4  operator-(const vec4& a) const;
    f32	  operator*(const vec4& a) const;
    vec4  operator*(f32 a) const;
    vec4  operator/(f32 a) const;
    vec4& operator+=(const vec4& a);
    vec4& operator-=(const vec4& a);
    vec4& operator/=(const vec4& a);
    vec4& operator/=(f32 a);
    vec4& operator*=(f32 a);

    bool operator==(const vec4& a) const;
    bool operator!=(const vec4& a) const;

    friend vec4 operator*(f32 a, vec4 b);

    vec4& set(f32 a, f32 b, f32 c, f32 d);
    vec4& zero();

    bool equal(const vec4& a) const;
    bool equal(const vec4& a, f32 epsilon) const;

    f32 length() const;
    f32 length_sqr() const;
    f32 dot(const vec4& a) const;
    vec4& normalize();
    vec4& truncate(f32 length);
    
    s32	dimension() const;

    const vec2& to_vec2() const;
    const vec3& to_vec3() const;
    vec2& to_vec2();
    vec3& to_vec3();

    f32*			ptr();
    const f32*	    ptr() const;
};

// Uses static 2d buffer internally, for debug purposes only.
const char* to_string(const vec3& v);

vec3 forward(const vec3& start, const vec3& end);
vec3 forward(f32 yaw, f32 pitch);
vec3 right(const vec3& start, const vec3& end, const vec3& up);

vec3 lerp(const vec3& a, const vec3& b, f32 alpha);
