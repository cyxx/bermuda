/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef DECODER_H__
#define DECODER_H__

#include "intern.h"

extern int decodeLzss(const uint8_t *src, uint8_t *dst);
extern int decodeZlib(const uint8_t *src, uint8_t *dst);

#endif // DECODER_H__
