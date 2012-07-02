/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef UTIL_H__
#define UTIL_H__

#include "intern.h"

enum {
	DBG_INFO     = 1 << 0,
	DBG_GAME     = 1 << 1,
	DBG_OPCODES  = 1 << 2,
	DBG_RES      = 1 << 3,
	DBG_DIALOGUE = 1 << 4,
	DBG_MIXER    = 1 << 5,
	DBG_WIN16    = 1 << 6
};

extern uint16_t g_debugMask;

extern void debug(uint16_t cm, const char *msg, ...);
extern void error(const char *msg, ...);
extern void warning(const char *msg, ...);

#endif // UTIL_H__
