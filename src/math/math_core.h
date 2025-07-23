#pragma once

inline constexpr f32 PI = 3.14159265358979323846f;

s32 Abs(s32 n);
f32 Abs(f32 n);
f32 Sqrt(f32 n);
f32 Sqrtr(f32 n);

f32 Rad(f32 d);
f32 Deg(f32 r);

f32 Cos(f32 r);
f32 Sin(f32 r);
f32 Tan(f32 r);

template<typename T>
T square(const T &n) {
	return n * n;
}

template<typename T>
T cube(const T &n) {
	return n * n * n;
}
