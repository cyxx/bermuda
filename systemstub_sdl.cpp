/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "mixer.h"
#include "systemstub.h"

enum {
	kSoundSampleRate = 22050,
	kSoundSampleSize = 4096,
	kVideoSurfaceDepth = 32,
};

struct SystemStub_SDL : SystemStub {
	Mixer *_mixer;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_Window *_window;
	SDL_Renderer *_renderer;
	SDL_Texture *_gameTexture;
	SDL_Texture *_videoTexture;
#else
	SDL_Surface *_screen;
	SDL_Overlay *_yuv;
#endif
	SDL_PixelFormat *_fmt;
	uint32_t *_gameBuffer;
	uint16_t *_videoBuffer;
	uint32_t _pal[256];
	int _screenW, _screenH;
	int _videoW, _videoH;
	bool _fullScreenDisplay;
	int _soundSampleRate;

	SystemStub_SDL() :
#if SDL_VERSION_ATLEAST(2, 0, 0)
		_window(0), _renderer(0), _gameTexture(0), _videoTexture(0),
#else
		_screen(0), _yuv(0),
#endif
		_fmt(0),
		_gameBuffer(0), _videoBuffer(0) {
		_mixer = Mixer_SDL_create(this);
	}
	virtual ~SystemStub_SDL() {
		delete _mixer;
	}

	virtual void init(const char *title, int w, int h);
	virtual void destroy();
	virtual void setPalette(const uint8_t *pal, int n);
	virtual void fillRect(int x, int y, int w, int h, uint8_t color);
	virtual void copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch, bool transparent);
	virtual void darkenRect(int x, int y, int w, int h);
	virtual void updateScreen();
	virtual void setYUV(bool flag, int w, int h);
	virtual uint8_t *lockYUV(int *pitch);
	virtual void unlockYUV();
	virtual void processEvents();
	virtual void sleep(int duration);
	virtual uint32_t getTimeStamp();
	virtual void lockAudio();
	virtual void unlockAudio();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual int getOutputSampleRate();
	virtual Mixer *getMixer() { return _mixer; }

	void handleEvent(const SDL_Event &ev, bool &paused);
	void setFullscreen(bool fullscreen);
};

SystemStub *SystemStub_SDL_create() {
	return new SystemStub_SDL();
}

#ifdef __EMSCRIPTEN__
static int eventHandler(void *userdata, SDL_Event *ev) {
	bool paused;
	((SystemStub_SDL *)userdata)->handleEvent(*ev, paused);
	return 0;
}
#endif

void SystemStub_SDL::init(const char *title, int w, int h) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_ShowCursor(SDL_DISABLE);
	_quit = false;
	memset(&_pi, 0, sizeof(_pi));

	_mixer->open();

#if SDL_VERSION_ATLEAST(2, 0, 0)
	_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
	SDL_GetWindowSize(_window, &_screenW, &_screenH);

	static const uint32_t pfmt = SDL_PIXELFORMAT_RGB888; //SDL_PIXELFORMAT_RGB565;
	_gameTexture = SDL_CreateTexture(_renderer, pfmt, SDL_TEXTUREACCESS_STREAMING, _screenW, _screenH);
	_fmt = SDL_AllocFormat(pfmt);
#else
	SDL_WM_SetCaption(title, NULL);
#endif

	_screenW = w;
	_screenH = h;

	const int bufferSize = _screenW * _screenH;
	_gameBuffer = (uint32_t *)calloc(bufferSize, sizeof(uint32_t));
	if (!_gameBuffer) {
		error("SystemStub_SDL::init() Unable to allocate offscreen buffer");
	}
	memset(_pal, 0, sizeof(_pal));

	_videoW = _videoH = 0;

	_fullScreenDisplay = false;
	setFullscreen(_fullScreenDisplay);
	_soundSampleRate = 0;

#ifdef __EMSCRIPTEN__
	emscripten_SDL_SetEventHandler(eventHandler, this);
#endif
}

void SystemStub_SDL::destroy() {
	_mixer->close();

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (_gameTexture) {
		SDL_DestroyTexture(_gameTexture);
		_gameTexture = 0;
	}
	if (_videoTexture) {
		SDL_DestroyTexture(_videoTexture);
		_videoTexture = 0;
	}
	if (_fmt) {
		SDL_FreeFormat(_fmt);
		_fmt = 0;
	}
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
#else
	if (_screen) {
		// free()'d by SDL_Quit()
		_screen = 0;
	}
#endif
	if (_gameBuffer) {
		free(_gameBuffer);
		_gameBuffer = 0;
	}
	SDL_Quit();
}

void SystemStub_SDL::setPalette(const uint8_t *pal, int n) {
	assert(n <= 256);
	_pal[0] = SDL_MapRGB(_fmt, 0, 0, 0); pal += 4;
	for (int i = 1; i < n; ++i) {
		_pal[i] = SDL_MapRGB(_fmt, pal[2], pal[1], pal[0]);
		pal += 4;
	}
}

static bool clipRect(int screenW, int screenH, int &x, int &y, int &w, int &h) {
	if (x < 0) {
		x = 0;
	}
	if (x + w > screenW) {
		w = screenW - x;
	}
	if (y < 0) {
		y = 0;
	}
	if (y + h > screenH) {
		h = screenH - y;
	}
	return (w > 0 && h > 0);
}

void SystemStub_SDL::fillRect(int x, int y, int w, int h, uint8_t color) {
	if (!clipRect(_screenW, _screenH, x, y, w, h)) return;

	const uint32_t fillColor = _pal[color];
	uint32_t *p = _gameBuffer + y * _screenW + x;
	while (h--) {
		for (int i = 0; i < w; ++i) {
			p[i] = fillColor;
		}
		p += _screenW;
	}
}

void SystemStub_SDL::copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch, bool transparent) {
	if (!clipRect(_screenW,  _screenH, x, y, w, h)) return;

	uint32_t *p = _gameBuffer + y * _screenW + x;
	buf += h * pitch;
	while (h--) {
		buf -= pitch;
		for (int i = 0; i < w; ++i) {
			if (!transparent || buf[i] != 0) {
				p[i] = _pal[buf[i]];
			}
		}
		p += _screenW;
	}
}

void SystemStub_SDL::darkenRect(int x, int y, int w, int h) {
	if (!clipRect(_screenW, _screenH, x, y, w, h)) return;

	const uint32_t redBlueMask = _fmt->Rmask | _fmt->Bmask;
	const uint32_t greenMask = _fmt->Gmask;

	uint32_t *p = _gameBuffer + y * _screenW + x;
	while (h--) {
		for (int i = 0; i < w; ++i) {
			uint32_t color = ((p[i] & redBlueMask) >> 1) & redBlueMask;
			color |= ((p[i] & greenMask) >> 1) & greenMask;
			p[i] = color;
		}
		p += _screenW;
	}
}

void SystemStub_SDL::updateScreen() {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_RenderClear(_renderer);
	SDL_UpdateTexture(_gameTexture, NULL, _gameBuffer, _screenW * sizeof(uint32_t));
	SDL_RenderCopy(_renderer, _gameTexture, NULL, NULL);
	SDL_RenderPresent(_renderer);
#else
	if (SDL_LockSurface(_screen) == 0) {
		for (int y = 0; y < _screenH; ++y) {
			uint8_t *dst = (uint8_t *)_screen->pixels + y * _screen->pitch;
			memcpy(dst, _gameBuffer + y * _screenW, _screenW * sizeof(uint32_t));
		}
		SDL_UnlockSurface(_screen);
		SDL_UpdateRect(_screen, 0, 0, 0, 0);
	}
#endif
}

void SystemStub_SDL::setYUV(bool flag, int w, int h) {
	if (flag) {
#ifndef __EMSCRIPTEN__
#if SDL_VERSION_ATLEAST(2, 0, 0)
		if (!_videoTexture) {
			_videoTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_UYVY, SDL_TEXTUREACCESS_STREAMING, w, h);
		}
		if (!_videoBuffer) {
			_videoBuffer = (uint16_t *)malloc(w * h * sizeof(uint16_t));
		}
#else
		if (!_yuv) {
			_yuv = SDL_CreateYUVOverlay(w, h, SDL_UYVY_OVERLAY, _screen);
		}
#endif
#endif
		_videoW = w;
		_videoH = w;
	} else {
#ifndef __EMSCRIPTEN__
#if SDL_VERSION_ATLEAST(2, 0, 0)
		if (_videoTexture) {
			SDL_DestroyTexture(_videoTexture);
			_videoTexture = 0;
		}
		if (_videoBuffer) {
			free(_videoBuffer);
			_videoBuffer = 0;
		}
#else
		if (_yuv) {
			SDL_FreeYUVOverlay(_yuv);
			_yuv = 0;
		}
#endif
#endif
	}
}

uint8_t *SystemStub_SDL::lockYUV(int *pitch) {
#ifndef __EMSCRIPTEN__
#if SDL_VERSION_ATLEAST(2, 0, 0)
	*pitch = _videoW * sizeof(uint16_t);
	return (uint8_t *)_videoBuffer;
#else
	if (_yuv && SDL_LockYUVOverlay(_yuv) == 0) {
		*pitch = _yuv->pitches[0];
		return _yuv->pixels[0];
	}
	return 0;
#endif
#else
	return 0;
#endif
}

void SystemStub_SDL::unlockYUV() {
#ifndef __EMSCRIPTEN__
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_RenderClear(_renderer);
	if (_videoBuffer) {
		SDL_UpdateTexture(_videoTexture, NULL, _videoBuffer, _videoW * sizeof(uint16_t));
	}
	SDL_RenderCopy(_renderer, _videoTexture, NULL, NULL);
	SDL_RenderPresent(_renderer);
#else
	if (_yuv) {
		SDL_UnlockYUVOverlay(_yuv);
		SDL_Rect r;
		if (_yuv->w * 2 <= _screenW && _yuv->h * 2 <= _screenH) {
			r.w = _yuv->w * 2;
			r.h = _yuv->h * 2;
		} else {
			r.w = _yuv->w;
			r.h = _yuv->h;
		}
		r.x = (_screenW - r.w) / 2;
		r.y = (_screenH - r.h) / 2;
		SDL_DisplayYUVOverlay(_yuv, &r);
	}
#endif
#endif
}

void SystemStub_SDL::processEvents() {
	bool paused = false;
	while (!_quit) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			handleEvent(ev, paused);
		}
		if (paused) {
			SDL_Delay(100);
		} else {
			break;
		}
	}
}

void SystemStub_SDL::handleEvent(const SDL_Event &ev, bool &paused) {
	switch (ev.type) {
	case SDL_QUIT:
		_quit = true;
		break;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	case SDL_WINDOWEVENT:
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			paused = (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST);
			SDL_PauseAudio(paused);
			break;
		}
		break;
#else
	case SDL_ACTIVEEVENT:
		if (ev.active.state & SDL_APPINPUTFOCUS) {
			paused = ev.active.gain == 0;
			SDL_PauseAudio(paused ? 1 : 0);
		}
		break;
#endif
	case SDL_KEYUP:
		switch (ev.key.keysym.sym) {
		case SDLK_LEFT:
			_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			break;
		case SDLK_RIGHT:
			_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			break;
		case SDLK_UP:
			_pi.dirMask &= ~PlayerInput::DIR_UP;
			break;
		case SDLK_DOWN:
			_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			break;
		case SDLK_RETURN:
			_pi.enter = false;
			break;
		case SDLK_SPACE:
			_pi.space = false;
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			_pi.shift = false;
			break;
		case SDLK_RCTRL:
		case SDLK_LCTRL:
			_pi.ctrl = false;
			break;
		case SDLK_TAB:
			_pi.tab = false;
			break;
		case SDLK_ESCAPE:
			_pi.escape = false;
			break;
		default:
			break;
		}
		break;
	case SDL_KEYDOWN:
	switch (ev.key.keysym.sym) {
		case SDLK_LEFT:
			_pi.dirMask |= PlayerInput::DIR_LEFT;
			break;
		case SDLK_RIGHT:
			_pi.dirMask |= PlayerInput::DIR_RIGHT;
			break;
		case SDLK_UP:
			_pi.dirMask |= PlayerInput::DIR_UP;
			break;
		case SDLK_DOWN:
			_pi.dirMask |= PlayerInput::DIR_DOWN;
			break;
		case SDLK_RETURN:
			_pi.enter = true;
			break;
		case SDLK_SPACE:
			_pi.space = true;
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			_pi.shift = true;
			break;
		case SDLK_RCTRL:
		case SDLK_LCTRL:
			_pi.ctrl = true;
			break;
		case SDLK_TAB:
			_pi.tab = true;
			break;
		case SDLK_ESCAPE:
			_pi.escape = true;
			break;
		case SDLK_f:
			_pi.fastMode = !_pi.fastMode;
			break;
		case SDLK_s:
			_pi.save = true;
			break;
		case SDLK_l:
			_pi.load = true;
			break;
		case SDLK_w:
			_fullScreenDisplay = !_fullScreenDisplay;
			setFullscreen(_fullScreenDisplay);
			break;
		case SDLK_KP_PLUS:
		case SDLK_PAGEUP:
			_pi.stateSlot = 1;
			break;
		case SDLK_KP_MINUS:
		case SDLK_PAGEDOWN:
			_pi.stateSlot = -1;
			break;
		default:
			break;
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (ev.button.button == SDL_BUTTON_LEFT) {
			_pi.leftMouseButton = true;
		} else if (ev.button.button == SDL_BUTTON_RIGHT) {
			_pi.rightMouseButton = true;
		}
		_pi.mouseX = ev.button.x;
		_pi.mouseY = ev.button.y;
		break;
	case SDL_MOUSEBUTTONUP:
		if (ev.button.button == SDL_BUTTON_LEFT) {
			_pi.leftMouseButton = false;
		} else if (ev.button.button == SDL_BUTTON_RIGHT) {
			_pi.rightMouseButton = false;
		}
		_pi.mouseX = ev.button.x;
		_pi.mouseY = ev.button.y;
		break;
	case SDL_MOUSEMOTION:
		_pi.mouseX = ev.motion.x;
		_pi.mouseY = ev.motion.y;
		break;
	default:
		break;
	}
}

void SystemStub_SDL::sleep(int duration) {
	SDL_Delay(duration);
}

uint32_t SystemStub_SDL::getTimeStamp() {
	return SDL_GetTicks();
}

void SystemStub_SDL::lockAudio() {
	SDL_LockAudio();
}

void SystemStub_SDL::unlockAudio() {
	SDL_UnlockAudio();
}

void SystemStub_SDL::startAudio(AudioCallback callback, void *param) {
	SDL_AudioSpec desired, obtained;
	memset(&desired, 0, sizeof(desired));
	desired.freq = kSoundSampleRate;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = kSoundSampleSize;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, &obtained) == 0) {
		_soundSampleRate = obtained.freq;
		SDL_PauseAudio(0);
	} else {
		error("SystemStub_SDL::startAudio() Unable to open sound device");
	}
}

void SystemStub_SDL::stopAudio() {
	SDL_CloseAudio();
}

int SystemStub_SDL::getOutputSampleRate() {
	return _soundSampleRate;
}

void SystemStub_SDL::setFullscreen(bool fullscreen) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetWindowFullscreen(_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#else
	_screen = SDL_SetVideoMode(_screenW, _screenH, kVideoSurfaceDepth, fullscreen ? SDL_FULLSCREEN : 0);
	if (_screen) {
		_fmt = _screen->format;
	}
#endif
}
