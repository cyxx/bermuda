/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include <ctime>
#include "random.h"

RandomGenerator::RandomGenerator() {
	uint16 seed = time(0);
	setSeed(seed);
}

void RandomGenerator::setSeed(uint16 seed) {
	_randomSeed = seed;
}

// Borland C random generator
uint16 RandomGenerator::getNumber() {
	const uint16 randomSeedLo = _randomSeed & 0xFFFF;
	const uint16 randomSeedHi = _randomSeed >> 16;
	uint16 rnd = 0x15A * randomSeedLo;
	if (randomSeedHi != 0) {
		rnd += 0x4E35 * randomSeedHi;
	}
	_randomSeed = (rnd << 16) | ((0x4E35 * randomSeedLo) & 0xFFFF);
	++_randomSeed;
	return _randomSeed & 0x7FFF;
}
