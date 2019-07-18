
#include <libretro.h>

#include "file.h"
#include "game.h"
#include "mixer.h"
#include "systemstub.h"

static const char *kName = "Bermuda Syndrome";
static const char *kVersion = "0.1.7";

static const int kAudioHz = 22050;
static const int kFps = 20;

static const int kMaxSaveStateSize = 128 * 1024;

static Game *g_game;
static char *g_dataPath;

// S16 stereo samples for each frame
static int16_t _audioBuffer[kAudioHz / kFps * 2];

static int _controllerDevicePort0 = RETRO_DEVICE_KEYBOARD;

struct SystemStub_libretro: SystemStub {

	uint32_t _palette[256];
	uint32_t *_offscreenBuffer;
	int _w, _h;
	Mixer *_mixer;
	AudioCallback _audioProc;
	void *_audioData;

	SystemStub_libretro() {
		_mixer = Mixer_Software_create(this);
		_audioProc = 0;
	}

	~SystemStub_libretro() {
		delete _mixer;
		_mixer = 0;
	}

	virtual void init(const char *title, int w, int h, bool fullscreen, int screenMode) {
		_offscreenBuffer = (uint32_t *)malloc(w * h * sizeof(uint32_t));
		_w = w;
		_h = h;
		_mixer->open();
	}
	virtual void destroy() {
		_mixer->close();
		free(_offscreenBuffer);
		_offscreenBuffer = 0;
		delete _mixer;
		_mixer = 0;
	}

	virtual void setIcon(const uint8_t *data, int size) {
	}
	virtual void showCursor(bool show) {
	}

	virtual void setPalette(const uint8_t *pal, int n) {
		for (int i = 0; i < n; ++i) {
			_palette[i] = (pal[2] << 16) | (pal[1] << 8) | pal[0];
			pal += 4;
		}
	}
	virtual void fillRect(int x, int y, int w, int h, uint8_t color) {
		assert(x >= 0 && x + w <= _w && y >= 0 && y + h <= _h);
		uint32_t *dst = _offscreenBuffer + y * _w + x;
		const uint32_t rgb = _palette[color];
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				dst[i] = rgb;
			}
			dst += w;
		}
	}
	virtual void copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch, bool transparent = false) {
		assert(x >= 0 && x + w <= _w && y >= 0 && y + h <= _h);
		uint32_t *dst = _offscreenBuffer + y * _w + x;
		buf += (h - 1) * pitch;
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				if (!transparent || buf[i] != 0) {
					dst[i] = _palette[buf[i]];
				}
			}
			dst += _w;
			buf -= pitch;
		}
        }
	virtual void darkenRect(int x, int y, int w, int h) {
		assert(x >= 0 && x + w <= _w && y >= 0 && y + h <= _h);
		const uint32_t redBlueMask = 0xFF00FF;
		const uint32_t greenMask = 0xFF00;
		uint32_t *dst = _offscreenBuffer + y * _w + x;
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				uint32_t color = ((dst[i] & redBlueMask) >> 1) & redBlueMask;
				color |= ((dst[i] & greenMask) >> 1) & greenMask;
				dst[i] = color;
			}
			dst += w;
		}
	}

	virtual void copyRectWidescreen(int w, int h, const uint8_t *buf, int pitch) {
	}
	virtual void clearWidescreen() {
	}
	virtual void updateScreen() {
	}

	virtual void setYUV(bool flag, int w, int h) {
	}
	virtual uint8_t *lockYUV(int *pitch) {
		return 0;
	}
	virtual void unlockYUV() {
	}

	virtual void processEvents() {
	}
	virtual void sleep(int duration) {
	}
	virtual uint32_t getTimeStamp() {
		return 0;
	}

	virtual void lockAudio() {
	}
	virtual void unlockAudio() {
	}
	virtual void startAudio(AudioCallback callback, void *param) {
		_audioProc = callback;
		_audioData = param;
	}
	virtual void stopAudio() {
		_audioProc = 0;
	}
	virtual int getOutputSampleRate() {
		return kAudioHz;
	}

	virtual Mixer *getMixer() {
		return _mixer;
	}
} g_stub;

static retro_pixel_format _pixelFormat = RETRO_PIXEL_FORMAT_XRGB8888;

struct retro_input_descriptor _inputDescriptors[] = {
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Use / Action" }, // enter
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Take Gun / Shoot" }, // space
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Holster Gun / Run" }, // shift
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Inventory" }, // tab
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Status" }, // ctrl
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Menu" }, // escape
	{ 0 }
};

static struct retro_variable _vars[] = {
	{ "no_hit", "Disable life countdown when hit; true|false" },
	{ 0, 0 }
};

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

void retro_set_environment(retro_environment_t cb) {
	environ_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
	video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
	audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
	input_state_cb = cb;
}

void retro_get_system_info(struct retro_system_info *info) {
	memset(info, 0, sizeof(*info));
	info->library_name     = kName;
	info->library_version  = kVersion;
	info->need_fullpath    = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
	memset(info, 0, sizeof(*info));
	info->timing.fps            = kFps;
	info->timing.sample_rate    = kAudioHz;
	info->geometry.base_width   = kGameScreenWidth;
	info->geometry.base_height  = kGameScreenHeight;
	info->geometry.max_width    = kGameScreenWidth;
	info->geometry.max_height   = kGameScreenHeight;
	info->geometry.aspect_ratio = 4.0f / 3.0f;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
	if (port == 0) {
		_controllerDevicePort0 = device;
	}
}

size_t retro_serialize_size() {
        return kMaxSaveStateSize;
}

bool retro_serialize(void *data, size_t size) {
	if (g_game->_state == kStateGame) {
		File f((uint8_t *)data, size);
		g_game->saveState(&f, 0);
		return !f.ioErr();
	}
	return false;
}

bool retro_unserialize(const void *data, size_t size) {
	if (g_game->_state == kStateGame) {
		File f((const uint8_t *)data, size);
		g_game->loadState(&f, 0, false);
		return !f.ioErr();
	}
	return false;
}

void retro_cheat_reset() {
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
}

bool retro_load_game(const struct retro_game_info *info) {

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, _inputDescriptors);
	environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, _vars);
	environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &_pixelFormat);

	g_debugMask = DBG_INFO; // | DBG_GAME | DBG_OPCODES | DBG_DIALOGUE;

	g_dataPath = strdup(info->path);
	const char *savePath = ".";
	g_game = new Game(&g_stub, g_dataPath, savePath, g_dataPath);
	g_game->init(false, SCREEN_MODE_DEFAULT);

	return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
	return false;
}

void retro_unload_game() {
	if (g_game) {
		delete g_game;
		g_game = 0;
	}
}

unsigned retro_get_region() {
	return RETRO_REGION_PAL;
}

unsigned retro_api_version() {
	return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id) {
	return 0;
}

size_t retro_get_memory_size(unsigned id) {
	return 0;
}

void retro_init() {
	struct retro_log_callback log;
	if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log)) {
		log_cb = log.log;
	}
}

void retro_deinit() {
}

void retro_reset() {
	g_game->restart();
}

static const struct {
	uint8_t player;
	uint8_t joypad;
	uint16_t keyboard;
} _directionMapping[] = {
	{ PlayerInput::DIR_UP, RETRO_DEVICE_ID_JOYPAD_UP, RETROK_UP },
	{ PlayerInput::DIR_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN, RETROK_DOWN },
	{ PlayerInput::DIR_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT, RETROK_LEFT },
	{ PlayerInput::DIR_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT, RETROK_RIGHT },
	{ 0, 0, 0 }
};

static struct {
	bool *player;
	uint8_t joypad;
	uint16_t keyboard;
} _buttonMapping[] = {
	{ 0, RETRO_DEVICE_ID_JOYPAD_A, RETROK_RETURN },
	{ 0, RETRO_DEVICE_ID_JOYPAD_B, RETROK_SPACE },
	{ 0, RETRO_DEVICE_ID_JOYPAD_X, RETROK_LSHIFT },
	{ 0, RETRO_DEVICE_ID_JOYPAD_Y, RETROK_TAB },
	{ 0, RETRO_DEVICE_ID_JOYPAD_SELECT, RETROK_LCTRL },
	{ 0, RETRO_DEVICE_ID_JOYPAD_START, RETROK_BACKSPACE },
	{ 0 }
};

static void updateInput(PlayerInput &pi) {
	_buttonMapping[0].player = &g_stub._pi.enter;
	_buttonMapping[1].player = &g_stub._pi.space;
	_buttonMapping[2].player = &g_stub._pi.shift;
	_buttonMapping[3].player = &g_stub._pi.tab;
	_buttonMapping[4].player = &g_stub._pi.ctrl;
	_buttonMapping[5].player = &g_stub._pi.escape;
	for (int i = 0; _buttonMapping[i].player; ++i) {
		if (_controllerDevicePort0 == RETRO_DEVICE_JOYPAD) {
			*_buttonMapping[i].player = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, _buttonMapping[i].joypad) != 0;
		} else {
			*_buttonMapping[i].player = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, _buttonMapping[i].keyboard) != 0;
		}
	}
	for (int i = 0; _directionMapping[i].player; ++i) {
		if (_controllerDevicePort0 == RETRO_DEVICE_JOYPAD) {
			if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, _directionMapping[i].joypad)) {
				pi.dirMask |= _directionMapping[i].player;
			} else {
				pi.dirMask &= ~_directionMapping[i].player;
			}
		} else {
			if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, _directionMapping[i].keyboard)) {
				pi.dirMask |= _directionMapping[i].player;
			} else {
				pi.dirMask &= ~_directionMapping[i].player;
			}
		}
	}
}

void retro_run() {
	bool updated = false;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
		retro_variable var;
		var.key = _vars[0].key; // 'no_hit'
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
			if (strcmp(var.value, "true") == 0) {
				g_game->_cheats |= kCheatNoHit;
			} else {
				g_game->_cheats &= ~kCheatNoHit;
			}
		}
	}
	updateInput(g_stub._pi);
	g_game->mainLoop();
	video_cb(g_stub._offscreenBuffer, g_stub._w, g_stub._h, g_stub._w * sizeof(uint32_t));
	g_stub._audioProc(g_stub._audioData, (uint8_t *)_audioBuffer, sizeof(_audioBuffer));
	audio_batch_cb(_audioBuffer, sizeof(_audioBuffer) / sizeof(int16_t));
}
