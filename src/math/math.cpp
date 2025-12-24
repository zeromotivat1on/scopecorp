#include "pch.h"
#include "vector.h"
#include "matrix.h"
#include "Quaternion.h"
#include "stb_sprintf.h"

// Vector2

Vector2 operator-(const Vector2 &a)                   { return { -a.x, -a.y }; }
Vector2 operator+(const Vector2 &a, const Vector2 &b) { return { a.x + b.x, a.y + b.y }; }
Vector2 operator-(const Vector2 &a, const Vector2 &b) { return { a.x - b.x, a.y - b.y }; }
Vector2 operator*(const Vector2 &a, f32 b)            { return { a.x * b, a.y * b }; }
Vector2 operator*(f32 a, const Vector2 &b)            { return b * a; }
f32  operator*(const Vector2 &a, const Vector2 &b)    { return dot(a, b); }
Vector2 operator/(const Vector2 &a, f32 b)            { return { a.x / b, a.y / b }; }
Vector2 operator/(const Vector2 &a, const Vector2 &b) { return { a.x / b.x, a.y / b.y }; }
Vector2 &operator+=(Vector2 &a, const Vector2 &b)     { a = a + b; return a; }
Vector2 &operator-=(Vector2 &a, const Vector2 &b)     { a = a - b; return a; }
Vector2 &operator/=(Vector2 &a, f32 b)                { a = a / b; return a; }
Vector2 &operator/=(Vector2 &a, const Vector2 &b)     { a = a / b; return a; }
Vector2 &operator*=(Vector2 &a, f32 b)                { a = a * b; return a; }
bool operator==(const Vector2 &a, const Vector2 &b)   { return equal(a, b); }
bool operator!=(const Vector2 &a, const Vector2 &b)   { return !(a == b); }

f32  length(const Vector2 &a)                         { return Sqrt(length_sqr(a)); }
f32  length_sqr(const Vector2 &a)                     { return a.x * a.x + a.y * a.y; }
f32  dot(const Vector2 &a, const Vector2 &b)          { return a.x * b.x + a.y * b.y; }
Vector2 normalize(const Vector2 &a)                   { return a / length(a); }
Vector2 direction(const Vector2 &a, const Vector2 &b) { return normalize(b - a); }
Vector2 abs(const Vector2 &a)                         { return { Abs(a.x), Abs(a.y) }; }

bool equal(const Vector2 &a, const Vector2 &b, f32 c) {
    return Abs(a.x - b.x) <= c && Abs(a.y - b.y) <= c;
}

Vector2 truncate(const Vector2 &a, f32 b) {
    if (b == 0.0f) return Vector2{0.0f, 0.0f};
    const f32 lensqr = length_sqr(a);
    if (lensqr > b * b) { const f32 s = b * Rsqrt(lensqr); return a * s; }
    return a;
}

String to_string(const Vector2 &a) { return tprint("(%.3f %.3f)", a.x, a.y); }

// Vector3

Vector3 operator-(const Vector3 &a)                   { return { -a.x, -a.y, -a.z }; }
Vector3 operator+(const Vector3 &a, const Vector3 &b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vector3 operator-(const Vector3 &a, const Vector3 &b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vector3 operator*(const Vector3 &a, f32 b)            { return { a.x * b, a.y * b, a.z * b }; }
Vector3 operator*(f32 a, const Vector3 &b)            { return b * a; }
f32  operator*(const Vector3 &a, const Vector3 &b)    { return dot(a, b); }
Vector3 operator/(const Vector3 &a, f32 b)            { return { a.x / b, a.y / b, a.z / b }; }
Vector3 operator/(const Vector3 &a, const Vector3 &b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }
Vector3 &operator+=(Vector3 &a, const Vector3 &b)   { a = a + b; return a; }
Vector3 &operator-=(Vector3 &a, const Vector3 &b)   { a = a - b; return a; }
Vector3 &operator/=(Vector3 &a, f32 b)              { a = a / b; return a; }
Vector3 &operator/=(Vector3 &a, const Vector3 &b)   { a = a / b; return a; }
Vector3 &operator*=(Vector3 &a, f32 b)              { a = a * b; return a; }
bool operator==(const Vector3 &a, const Vector3 &b) { return equal(a, b); }
bool operator!=(const Vector3 &a, const Vector3 &b) { return !(a == b); }

f32  length(const Vector3 &a)                         { return Sqrt(length_sqr(a)); }
f32  length_sqr(const Vector3 &a)                     { return a.x * a.x + a.y * a.y + a.z * a.z; }
f32  dot(const Vector3 &a, const Vector3 &b)          { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vector3 normalize(const Vector3 &a)                   { return a / length(a); }
Vector3 direction(const Vector3 &a, const Vector3 &b) { return normalize(b - a); }
Vector3 abs(const Vector3 &a)                         { return Vector3(Abs(a.x), Abs(a.y), Abs(a.z)); }

bool equal(const Vector3 &a, const Vector3 &b, f32 c) {
    return Abs(a.x - b.x) <= c && Abs(a.y - b.y) <= c && Abs(a.z - b.z) <= c;
}

Vector3 truncate(const Vector3&a, f32 b) {
    if (b == 0.0f) return Vector3{0.0f, 0.0f, 0.0f};
    const f32 lensqr = length_sqr(a);
    if (lensqr > b * b) { const f32 s = b * Rsqrt(lensqr); return a * s; }
    return a;
}

Vector3 cross(const Vector3 &a, const Vector3 &b) {
    return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

String to_string(const Vector3 &a) { return tprint("(%.3f %.3f %.3f)", a.x, a.y, a.z); }

Vector3 forward(f32 yaw, f32 pitch) {
    const f32 ycos = Cos(To_Radians(yaw));
    const f32 ysin = Sin(To_Radians(yaw));
    const f32 pcos = Cos(To_Radians(pitch));
    const f32 psin = Sin(To_Radians(pitch));
    return normalize(Vector3(ycos * pcos, psin, ysin * pcos));
}

Vector3 right(const Vector3 &start, const Vector3 &end, const Vector3 &up) {
    return normalize(cross(up, end - start));
}

// Vector4

Vector4 operator-(const Vector4 &a)                   { return { -a.x, -a.y, -a.z, -a.w }; }
Vector4 operator+(const Vector4 &a, const Vector4 &b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
Vector4 operator-(const Vector4 &a, const Vector4 &b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
Vector4 operator*(const Vector4 &a, f32 b)            { return { a.x * b, a.y * b, a.z * b, a.w * b }; }
Vector4 operator*(f32 a, const Vector4 &b)            { return b * a; }
f32  operator*(const Vector4 &a, const Vector4 &b)    { return dot(a, b); }
Vector4 operator/(const Vector4 &a, f32 b)            { return { a.x / b, a.y / b, a.z / b, a.w / b }; }
Vector4 operator/(const Vector4 &a, const Vector4 &b) { return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
Vector4 &operator+=(Vector4 &a, const Vector4 &b)     { a = a + b; return a; }
Vector4 &operator-=(Vector4 &a, const Vector4 &b)     { a = a - b; return a; }
Vector4 &operator/=(Vector4 &a, f32 b)                { a = a / b; return a; }
Vector4 &operator/=(Vector4 &a, const Vector4 &b)     { a = a / b; return a; }
Vector4 &operator*=(Vector4 &a, f32 b)                { a = a * b; return a; }
bool operator==(const Vector4 &a, const Vector4 &b)   { return equal(a, b); }
bool operator!=(const Vector4 &a, const Vector4 &b)   { return !(a == b); }

f32  length(const Vector4 &a)                         { return Sqrt(length_sqr(a)); }
f32  length_sqr(const Vector4 &a)                     { return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w; }
f32  dot(const Vector4 &a, const Vector4 &b)          { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
Vector4 normalize(const Vector4 &a)                   { return a / length(a); }
Vector4 direction(const Vector4 &a, const Vector4 &b) { return normalize(b - a); }
Vector4 abs(const Vector4 &a)                         { return { Abs(a.x), Abs(a.y), Abs(a.z), Abs(a.w) }; }

bool equal(const Vector4 &a, const Vector4 &b, f32 c) {
    return Abs(a.x - b.x) <= c && Abs(a.y - b.y) <= c && Abs(a.z - b.z) <= c && Abs(a.w - b.w) <= c;
}

Vector4 truncate(const Vector4&a, f32 b) {
    if (b == 0.0f) return Vector4{0.0f, 0.0f, 0.0f, 0.0f};
    const f32 lensqr = length_sqr(a);
    if (lensqr > b * b) { const f32 s = b * Rsqrt(lensqr); return a * s; }
    return a;
}

String to_string(const Vector4 &a) { return tprint("(%.3f %.3f %.3f %.3f)", a.x, a.y, a.z, a.w); }

// Matrix2

Matrix2 Matrix2_identity() {
	return Matrix2(Vector2(1, 0), Vector2(0, 1));
}

Matrix2 operator-(const Matrix2 &a)                   { return { -a[0], -a[1] }; }
Matrix2 operator+(const Matrix2 &a, const Matrix2 &b) { return { a[0] + b[0], a[1] + b[1] }; }
Matrix2 operator-(const Matrix2 &a, const Matrix2 &b) { return { a[0] - b[0], a[1] - b[1] }; }

Matrix2 operator*(const Matrix2 &a, const Matrix2 &b) {
	return { a[0][0] * b[0][0] + a[0][1] * b[1][0],
             a[0][0] * b[0][1] + a[0][1] * b[1][1],
             a[1][0] * b[0][0] + a[1][1] * b[1][0],
             a[1][0] * b[0][1] + a[1][1] * b[1][1], };
}

Matrix2 operator*(const Matrix2 &a, f32 b) { return { a[0] * b, a[1] * b }; }

Vector2 operator*(const Matrix2 &a, const Vector2 &b) {
	return { a[0][0] * b[0] + a[0][1] * b[1],
             a[1][0] * b[0] + a[1][1] * b[1] };
}

Matrix2 &operator+=(Matrix2 &a, const Matrix2 &b) { a = a + b; return a; }
Matrix2 &operator-=(Matrix2 &a, const Matrix2 &b) { a = a - b; return a; }
Matrix2 &operator*=(Matrix2 &a, const Matrix2 &b) { a = a * b; return a; }
Matrix2 &operator*=(Matrix2 &a, f32 b)             { a = a * b; return a; }

bool operator==(const Matrix2 &a, const Matrix2 &b) { return equal(a, b); }
bool operator!=(const Matrix2 &a, const Matrix2 &b) { return !(a == b); }

Matrix2 operator*(f32 a, const Matrix2 &b)            { return b * a; }
Vector2 operator*(const Vector2 &a, const Matrix2 &b) { return b * a; }
Vector2 &operator*=(Vector2 &a, const Matrix2 &b)     { a = a * b; return a; }

bool equal(const Matrix2 &a, const Matrix2 &b, f32 c) {
	return equal(a[0], b[0], c) && equal(a[1], b[1], c);
}

bool identity (const Matrix2 &a, f32 b) { return equal(a, Matrix2_identity(), b); }
bool symmetric(const Matrix2 &a, f32 b) { return Abs(a[0][1] - a[1][0]) < b; }
bool diagonal (const Matrix2 &a, f32 b) { return Abs(a[0][1]) <= b && Abs(a[1][0]) <= b; }

Matrix2 transpose(const Matrix2 &a) { return { a[0][0], a[1][0], a[0][1], a[1][1] }; }
f32     trace    (const Matrix2 &a) { return a[0][0] + a[1][1]; }

// Matrix3

Matrix3::Matrix3(const Quaternion &a) {
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

Matrix3 Matrix3_identity() {
	return { Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1) };
}

Matrix3 operator-(const Matrix3 &a)                   { return { -a[0], -a[1], -a[2] }; }
Matrix3 operator+(const Matrix3 &a, const Matrix3 &b) { return { a[0] + b[0], a[1] + b[1], a[2] + b[2] }; }
Matrix3 operator-(const Matrix3 &a, const Matrix3 &b) { return { a[0] - b[0], a[1] - b[1], a[2] - b[2] }; }

Matrix3 operator*(const Matrix3 &a, const Matrix3 &b) {
	Matrix3 r;
    
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

Matrix3 operator*(const Matrix3 &a, f32 b) { return { a[0] * b, a[1] * b, a[2] * b }; }

Vector3 operator*(const Matrix3 &a, const Vector3 &b) {
	return { a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2],
             a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2],
             a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2], };
}

Matrix3 &operator+=(Matrix3 &a, const Matrix3 &b) { a = a + b; return a; }
Matrix3 &operator-=(Matrix3 &a, const Matrix3 &b) { a = a - b; return a; }
Matrix3 &operator*=(Matrix3 &a, const Matrix3 &b) { a = a * b; return a; }
Matrix3 &operator*=(Matrix3 &a, f32 b)            { a = a * b; return a; }

bool operator==(const Matrix3 &a, const Matrix3 &b) { return equal(a, b); }
bool operator!=(const Matrix3 &a, const Matrix3 &b) { return !(a == b); }

Matrix3 operator*(f32 a, const Matrix3 &b)            { return b * a; }
Vector3 operator*(const Vector3 &a, const Matrix3 &b) { return b * a; }
Vector3 &operator*=(Vector3 &a, const Matrix3 &b)     { a = a * b; return a; }

bool equal(const Matrix3 &a, const Matrix3 &b, f32 c) {
	return equal(a[0], b[0], c) && equal(a[1], b[1], c) && equal(a[2], b[2], c);
}

bool identity(const Matrix3 &a, f32 b) { return equal(a, Matrix3_identity(), b); }

bool symmetric(const Matrix3 &a, f32 b) {
    return Abs(a[0][1] - a[1][0]) <= b
        && Abs(a[0][2] - a[2][0]) <= b
        && Abs(a[1][2] - a[2][1]) <= b;
}

bool diagonal(const Matrix3 &a, f32 b) {
    return Abs(a[0][1]) <= b
        && Abs(a[0][2]) <= b
        && Abs(a[1][0]) <= b
        && Abs(a[1][2]) <= b
        && Abs(a[2][0]) <= b
        && Abs(a[2][1]) <= b;
}

bool rotated(const Matrix3 &a) {
	return a[0][1] != 0.0f || a[0][2] != 0.0f
        || a[1][0] != 0.0f || a[1][2] != 0.0f
        || a[2][0] != 0.0f || a[2][1] != 0.0f;
}

Matrix3 transpose(const Matrix3 &a) {
	return { a[0][0], a[1][0], a[2][0],
             a[0][1], a[1][1], a[2][1],
             a[0][2], a[1][2], a[2][2], };
}

f32 trace(const Matrix3 &a) { return a[0][0] + a[1][1] + a[2][2]; }

// Matrix4

Matrix4 Matrix4_identity() {
	return { Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1) };
}

Matrix4 operator-(const Matrix4 &a)                   { return { -a[0], -a[1], -a[2], -a[3] }; }
Matrix4 operator+(const Matrix4 &a, const Matrix4 &b) { return { a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3] }; }
Matrix4 operator-(const Matrix4 &a, const Matrix4 &b) { return { a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3] }; }

Matrix4 operator*(const Matrix4 &a, const Matrix4 &b) {
	Matrix4 r;
    
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

Matrix4 operator*(const Matrix4 &a, f32 b) { return { a[0] * b, a[1] * b, a[2] * b, a[3] * b }; }

Vector4 operator*(const Matrix4 &a, const Vector4 &b) {
	return { a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3] * b[3],
             a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3] * b[3],
             a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3] * b[3],
             a[3][0] * b[0] + a[3][1] * b[1] + a[3][2] * b[2] + a[3][3] * b[3], };
}

Vector3 operator*(const Matrix4 &a, const Vector3 &b) {
	const f32 s = a[3][0] * b[0] + a[3][1] * b[1] + a[3][2] * b[2] + a[3][3];

	if (s == 0.0f) return Vector3_zero;

	if (s == 1.0f) {
		return { a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3],
                 a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3],
                 a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3], };
	}

	const f32 sr = 1.0f / s;
    return { (a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3]) * sr,
             (a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3]) * sr,
             (a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3]) * sr, };
}

Matrix4 &operator+=(Matrix4 &a, const Matrix4 &b) { a = a + b; return a; }
Matrix4 &operator-=(Matrix4 &a, const Matrix4 &b) { a = a - b; return a; }
Matrix4 &operator*=(Matrix4 &a, const Matrix4 &b) { a = a * b; return a; }
Matrix4 &operator*=(Matrix4 &a, f32 b)            { a = a * b; return a; }

bool operator==(const Matrix4 &a, const Matrix4 &b) { return equal(a, b); }
bool operator!=(const Matrix4 &a, const Matrix4 &b) { return !(a == b); }

Matrix4 operator*(f32 a, const Matrix4 &b)            { return b * a; }
Vector3 operator*(const Vector3 &a, const Matrix4 &b) { return b * a; }
Vector4 operator*(const Vector4 &a, const Matrix4 &b) { return b * a; }

Vector3 &operator*=(Vector3 &a, const Matrix4 &b) { a = b * a; return a; }
Vector4 &operator*=(Vector4 &a, const Matrix4 &b) { a = b * a; return a; }

bool equal(const Matrix4 &a, const Matrix4 &b, f32 c) {
	return equal(a[0], b[0], c)
        && equal(a[1], b[1], c)
        && equal(a[2], b[2], c)
        && equal(a[3], b[3], c);
}

bool identity(const Matrix4 &a, f32 b) { return equal(a, Matrix4_identity(), b); }

bool symmetric(const Matrix4 &a, f32 b) {
	for (s32 i = 1; i < 4; ++i)
		for (s32 j = 0; j < i; ++j)
			if (Abs(a[i][j] - a[j][i]) > b) return false;
	return true;
}

bool diagonal(const Matrix4 &a, f32 b) {
	for (s32 i = 0; i < 4; ++i)
		for (s32 j = 0; j < 4; ++j) 
			if (i != j && Abs(a[i][j]) > b)  return false;
	return true;
}

bool rotated(const Matrix4 &a) {
	return a[0][1] != 0.0f || a[0][2] != 0.0f
        || a[1][0] != 0.0f || a[1][2] != 0.0f
        || a[2][0] != 0.0f || a[2][1] != 0.0f;
}

Matrix4 &identity(Matrix4 &a) { a = Matrix4_identity(); return a; }

Matrix4 transpose(const Matrix4 &a) {
	Matrix4 r;
	for(s32 i = 0; i < 4; ++i)
		for(s32 j = 0; j < 4; ++j)
			r[i][j] = a[j][i];

	return r;
}

f32 trace(const Matrix4 &a) { return a[0][0] + a[1][1] + a[2][2] + a[3][3]; }

Matrix4 translate(const Matrix4 &a, const Vector3 &b) {
    auto r = a;
    r[3][0] = b.x;
    r[3][1] = b.y;
    r[3][2] = b.z;
    return r;
}

Matrix4 rotate(const Matrix4 &a, const Matrix4 &b)    { return a * b; }
Matrix4 rotate(const Matrix4 &a, const Quaternion &b) { return a * Matrix4(b); }

Matrix4 scale(const Matrix4 &a, const Vector3 &b) {
    Matrix4 r = a; r[0][0] = b.x; r[1][1] = b.y; r[2][2] = b.z; return r;
}

Matrix4 make_transform(const Vector3 &t, const Quaternion &r, const Vector3 &s) {
    return translate(rotate(scale(Matrix4_identity(), s), r), t);
}

Matrix4 make_view(const Vector3 &eye, const Vector3 &at, const Vector3 &up) {
	const auto f = normalize(at - eye);
	const auto s = normalize(cross(up, f));
	const auto u = cross(f, s);

	Matrix4 a;

	a[0][0] = s.x;
	a[1][0] = s.y;
	a[2][0] = s.z;
	a[3][0] = -dot(s, eye);

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

Matrix4 make_perspective(f32 rfovy, f32 aspect, f32 n, f32 f) {
	const f32 tan_half_fovy = Tan(rfovy * 0.5f);

	Matrix4 a;
	a[0][0] = 1.0f / (aspect * tan_half_fovy); // x-axis scaling
	a[1][1] = 1.0f / tan_half_fovy;			   // y-axis scaling
	a[2][2] = -(f + n) / (f - n);			   // z-axis scaling
	a[2][3] = -1.0f;						   // rh perspective divide
	a[3][2] = -(2.0f * f * n) / (f - n);	   // z-axis translation

	return a;
}

Matrix4 make_orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	Matrix4 a;
	a[0][0] =  2.0f / (r - l);
	a[1][1] =  2.0f / (t - b);
	a[2][2] = -2.0f / (f - n);
	a[3][0] = -(r + l) / (r - l);
	a[3][1] = -(t + b) / (t - b);
	a[3][2] = -(f + n) / (f - n);
	a[3][3] =  1.0f;
	return a;
}

Matrix4 inverse(const Matrix4 &m) {
    auto minv = m;
    
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

	if (Abs(det) < MATRIX_INV_EPSILON) {
        log(LOG_MINIMAL, "Failed to inverse Matrix4");
		return Matrix4_identity();
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

// Quaternion

Quaternion operator-(const Quaternion &a)                      { return { -a.x, -a.y, -a.z, -a.w }; }
Quaternion operator+(const Quaternion &a, const Quaternion &b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
Quaternion operator-(const Quaternion &a, const Quaternion &b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }

Quaternion operator*(const Quaternion &a, const Quaternion &b) {
	return { a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
             a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
             a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
             a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, };
}

Vector3 operator*(const Quaternion &a, const Vector3 &b) {
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
    
	return { (xxzz + wwyy) * b.x + (xy2 + zw2)         * b.y + (xz2 - yw2)   * b.z,
             (xy2 - zw2)   * b.x + (yy + ww - xx - zz) * b.y + (yz2 + xw2)   * b.z,
             (xz2 + yw2)   * b.x + (yz2 - xw2)         * b.y + (wwyy - xxzz) * b.z, };
}

Quaternion operator*(const Quaternion &a, f32 b) { return { a.x * b, a.y * b, a.z * b, a.w * b }; }

Quaternion &operator+=(Quaternion &a, const Quaternion &b) { a = a + b; return a; }
Quaternion &operator-=(Quaternion &a, const Quaternion &b) { a = a - b; return a; }
Quaternion &operator*=(Quaternion &a, const Quaternion &b) { a = a * b; return a; } 
Quaternion &operator*=(Quaternion &a, f32 b)               { a = a * b; return a; }

bool operator==(const Quaternion &a, const Quaternion &b) { return equal(a, b); }
bool operator!=(const Quaternion &a, const Quaternion &b) { return !(a == b); }

Quaternion operator*(const f32 a, const Quaternion &b)   { return b * a; }
Vector3 operator*(const Vector3 &a, const Quaternion &b) { return b * a; }

bool equal(const Quaternion &a, const Quaternion &b, const f32 epsilon) {
	return Abs(a.x - b.x) <= epsilon
        && Abs(a.y - b.y) <= epsilon
        && Abs(a.z - b.z) <= epsilon
        && Abs(a.w - b.w) <= epsilon;
}

Quaternion inverse(const Quaternion &a) { return { -a.x, -a.y, -a.z, a.w }; }
f32        length (const Quaternion &a)  { return Sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w); }

Quaternion normalize(const Quaternion &a) {
    Quaternion r = a;
	const f32 l = length(a);
	if (l != 0.0f) {
		const f32 lr = 1 / l;
		r.x *= lr; r.y *= lr; r.z *= lr; r.w *= lr;
	}
	return r;
}

Vector3 forward(const Quaternion &a) {
    Vector3 v;
    v.x = 0 + 2 * (a.x * a.z + a.w * a.y);
    v.y = 0 + 2 * (a.y * a.z - a.w * a.x);
    v.z = 1 - 2 * (a.x * a.x + a.y * a.y);
    return normalize(v);
}

f32 calc_w(const Quaternion &a) { return Sqrt(Abs(1.0f - (a.x * a.x + a.y * a.y + a.z * a.z))); }

String to_string(const Quaternion &q) { return tprint("(%.3f %.3f %.3f %.3f)", q.x, q.y, q.z, q.w); }

Quaternion make_quaternion_from_axis_angle(const Vector3 &axes, f32 deg) {
    const f32 angle  = To_Radians(deg * 0.5f);
    const f32 factor = Sin(angle);

    const f32 x = axes.x * factor;
    const f32 y = axes.y * factor;
    const f32 z = axes.z * factor;

    const f32 w = Cos(angle);

    return normalize(Quaternion(x, y, z, w));
}
