/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef MIXER_H__
#define MIXER_H__

#include "intern.h"

struct File;
struct SystemStub;

struct Mixer {
	static const int kDefaultSoundId = -1;

	Mixer() {}
	virtual ~Mixer() {}

	virtual void open() = 0;
	virtual void close() = 0;

	virtual void playSound(File *f, int *id) = 0;
	virtual void playMusic(File *f, int *id) = 0;
	virtual bool isSoundPlaying(int id) = 0;
	virtual void stopSound(int id) = 0;
	virtual void stopAll() = 0;

	virtual void setMusicMix(void *param, void (*mix)(void *, uint8_t *, int)) = 0;
};

Mixer *Mixer_SDL_create(SystemStub *);
Mixer *Mixer_Software_create(SystemStub *);

#endif // MIXER_H__
