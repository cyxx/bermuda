/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef INTERN_H__
#define INTERN_H__

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <stdint.h>

#include "util.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

struct Point {
	int x, y;
};

struct Rect {
	int x, y;
	int w, h;
};

inline uint16_t READ_LE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[1] << 8) | b[0];
}

inline uint32_t READ_LE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

inline uint16_t READ_BE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 8) | b[1];
}

inline uint32_t READ_BE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

#ifndef MIN
template<typename T>
inline T MIN(T v1, T v2) {
	return (v1 < v2) ? v1 : v2;
}
#endif

#ifndef MAX
template<typename T>
inline T MAX(T v1, T v2) {
	return (v1 > v2) ? v1 : v2;
}
#endif

template<typename T>
inline T ABS(T t) {
	return (t < 0) ? -t : t;
}

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a; a = b; b = tmp;
}

template<typename T>
inline T CLIP(T t, T tmin, T tmax) {
	if (t < tmin) {
		return tmin;
	} else if (t > tmax) {
		return tmax;
	} else {
		return t;
	}
}

#endif // INTERN_H__
