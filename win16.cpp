/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "file.h"
#include "game.h"
#include "mixer.h"
#include "systemstub.h"

int Game::win16_sndPlaySound(int op, void *data) {
	debug(DBG_WIN16, "win16_sndPlaySound() %d", op);
	switch (op) {
	case 22:
		if (!_mixer->isSoundPlaying(_mixerSoundId)) {
			return 1;
		}
		break;
	case 3: {
			const char *fileName = (const char *)data;
			File *f = _fs.openFile(fileName, false);
			if (f) {
				_mixer->playSound(f, &_mixerSoundId);
				_fs.closeFile(f);
			} else {
				warning("Unable to open wav file '%s'", fileName);
			}
		}
		break;
	case 6:
	case 7:
		_mixer->stopSound(_mixerSoundId);
		break;
	default:
		warning("Unhandled op %d in win16_sndPlaySound", op);
		break;
	}
	return 0;
}

void Game::win16_stretchBits(SceneBitmap *bits, int srcHeight, int srcWidth, int srcY, int srcX, int dstHeight, int dstWidth, int dstY, int dstX) {
	debug(DBG_WIN16, "win16_stretchBits() %d,%d %d,%d", srcX, srcY, srcWidth, srcHeight);
	assert(srcWidth == dstWidth && srcHeight == dstHeight);
	const uint8_t *src = bits->bits + srcY * bits->pitch + srcX;
	if (dstX >= kGameScreenWidth) {
		return;
	} else if (dstX + dstWidth > kGameScreenWidth) {
		dstWidth = kGameScreenWidth - dstX;
	}
	if (dstY >= kGameScreenHeight) {
		return;
	} else if (dstY + dstHeight > kGameScreenHeight) {
		dstHeight = kGameScreenHeight - dstY;
	}
	_stub->copyRect(dstX, dstY, dstWidth, dstHeight, src, bits->pitch);
}
