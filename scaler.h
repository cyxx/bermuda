
#ifndef SCALER_H__
#define SCALER_H__

#include <stdint.h>

#define SCALER_TAG 1

struct Scaler {
	int tag;
	const char *name;
	int factorMin, factorMax;
	void (*scale32)(int factor, const uint32_t *src, uint32_t *dst, int w, int h);
};

extern "C" {
	const Scaler *getScaler();
}

#endif
