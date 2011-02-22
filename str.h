/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef STR_H__
#define STR_H__

#include "intern.h"

extern void stringToLowerCase(char *p);
extern void stringToUpperCase(char *p);
extern char *stringTrimRight(char *p);
extern char *stringTrimLeft(char *p);
extern char *stringNextToken(char **p);
extern char *stringNextTokenEOL(char **p);
extern void stringStripComments(char *p);
extern bool stringEndsWith(const char *p, const char *suf);

#endif // STR_H__
