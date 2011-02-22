/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef RANDOM_H__
#define RANDOM_H__

#include "intern.h"

struct RandomGenerator {
	RandomGenerator();

	void setSeed(uint16 seed);
	uint16 getNumber();

	uint32 _randomSeed;
};

#endif // RANDOM_H__
