﻿#pragma once

inline constexpr f32 PI = 3.14159265358979323846f;

s32 absi(s32 n);
f32 absf(f32 n);
f32 sqrt(f32 n);
f32 sqrt_inv(f32 n);

f32 rad(f32 d);
f32 deg(f32 r);

f32 cos(f32 r);
f32 sin(f32 r);
f32 tan(f32 r);

template<typename T>
T square(const T &n) {
	return n * n;
}

template<typename T>
T cube(const T &n) {
	return n * n * n;
}

template<typename T>
void swap(T &a, T &b) {
	T temp = a;
	a = b;
	b = temp;
}

template<typename T>
T lerp(const T &a, const T &b, f32 alpha) {
    if (alpha <= 0.0f) return a;
    else if (alpha >= 1.0f) return b;
    return a + alpha * (b - a);
}
