/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef RANDOM_H__
#define RANDOM_H__

#include "intern.h"

struct RandomGenerator {
	RandomGenerator();

	void setSeed(uint16_t seed);
	uint16_t getNumber();

	uint32_t _randomSeed;
};

#endif // RANDOM_H__
