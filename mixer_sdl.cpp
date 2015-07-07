
#include <SDL.h>
#include <SDL_mixer.h>
#include "file.h"
#include "mixer.h"
#include "util.h"

struct MixerSDL: Mixer {

	static const int kMixFreq = 22050;
	static const int kMixBufSize = 4096;
	static const int kChannels = 4;

	bool _isOpen;
	Mix_Music *_music;

	MixerSDL()
		: _isOpen(false), _music(0) {
	}

	virtual ~MixerSDL() {
	}

	virtual void open() {
		if (_isOpen) {
			return;
		}
		Mix_Init(MIX_INIT_OGG | MIX_INIT_FLUIDSYNTH);
		if (Mix_OpenAudio(kMixFreq, AUDIO_S16SYS, 2, kMixBufSize) < 0) {
			warning("Mix_OpenAudio failed: %s", Mix_GetError());
		}
		Mix_AllocateChannels(kChannels);
		_isOpen = true;
	}
	virtual void close() {
		if (!_isOpen) {
			return;
		}
		stopAll();
		Mix_CloseAudio();
		Mix_Quit();
		_isOpen = false;
	}

	virtual void playSound(File *f, int *id) {
		debug(DBG_MIXER, "MixerSDL::playSound() path '%s'", f->_path);
		Mix_Chunk *chunk = Mix_LoadWAV(f->_path);
		if (chunk) {
			*id = Mix_PlayChannel(-1, chunk, 0);
		} else {
			*id = -1;
		}
	}
	virtual void playMusic(File *f, int *id) {
		debug(DBG_MIXER, "MixerSDL::playSoundMusic() path '%s'", f->_path);
		playMusic(f->_path);
		*id = -1;
	}
	virtual bool isSoundPlaying(int id) {
		return Mix_Playing(id) != 0;
	}
	virtual void stopSound(int id) {
		debug(DBG_MIXER, "MixerSDL::stopSound()");
		Mix_HaltChannel(id);
		// Mix_FreeChunk
	}

	void playMusic(const char *path) {
		stopMusic();
		_music = Mix_LoadMUS(path);
		if (_music) {
			Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
			Mix_PlayMusic(_music, 0);
		} else {
			warning("Failed to load music '%s', %s", path, Mix_GetError());
		}
	}
	void stopMusic() {
		Mix_HaltMusic();
		Mix_FreeMusic(_music);
		_music = 0;
	}
	virtual void setMusicMix(void *param, void (*mix)(void *, uint8_t *, int)) {
#ifndef __EMSCRIPTEN__
		if (mix) {
			Mix_HookMusic(mix, param);
		} else {
			Mix_HookMusic(0, 0);
		}
#endif
	}

	virtual void stopAll() {
		debug(DBG_MIXER, "MixerSDL::stopAll()");
		Mix_HaltChannel(-1);
		stopMusic();
	}
};

Mixer *Mixer_SDL_create(SystemStub *stub) {
	return new MixerSDL();
}
