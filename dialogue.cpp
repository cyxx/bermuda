/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "decoder.h"
#include "file.h"
#include "game.h"
#include "str.h"
#include "systemstub.h"

static const Rect _dialogueBackgroundRect = { 33, 165, 574, 150 };
static const int _selectedDialogueChoiceColor = 0xCAF6FF;
static const int _unselectedDialogueChoiceColor = 0x6DC6FA;

static void drawChar(uint8_t *dst, int dstPitch, const uint16_t *fontData, int c, uint8_t color) {
	int offset = c * 16;
	for (int i = 0; i < 16; ++i) {
		uint16_t chr = fontData[offset + i];
		for (int b = 0; b < 16; ++b) {
			if (chr & (1 << b)) {
				dst[-i * dstPitch + b] = color;
			}
		}
	}
}

static int getHangulGlyphOffset(int codeLo, int codeHi) {
	int offset, num;
	if (codeHi == 0x20) {
		return codeLo * 7;
	} else if (codeLo <= 0xAC) {
		offset = 128 * 7;
		num = codeLo - 0xA1;
	} else if (codeLo <= 0xC8) {
		offset = 1256 * 7;
		num = codeLo - 0xB0;
	} else {
		offset = 3606 * 7;
		num = codeLo - 0xCA;
	}
	return offset + (num * 94 + (codeHi - 0xA1)) * 7;
}

static void drawHangulGlyph(uint8_t *dst, int dstPitch, const uint8_t *fontData, uint32_t fontLutOffset, int c, uint8_t color) {
	if (!fontData) return;
	int offset = getHangulGlyphOffset(c >> 8, c & 0xFF);

	const uint8_t *p = fontData + fontLutOffset + offset;
	const int bufferIndex = READ_LE_UINT16(p);
	const int bufferOffset = READ_LE_UINT16(p + 2);

	p = fontData + 38 + bufferIndex * 16;
	offset = READ_LE_UINT32(p);
	assert(bufferOffset < READ_LE_UINT16(p + 4));
	p = fontData + offset + bufferOffset;

	for (int y = 0; y < 16; ++y, p += 2) {
		const int bits = p[0] * 256 + p[1];
		for (int x = 0; x < 16; ++x) {
			if ((bits & (1 << (15 - x))) == 0) {
				dst[-y * dstPitch + x] = color;
			}
		}
	}
}

static uint8_t findBestMatchingColor(const uint8_t *src, int color) {
	uint8_t bestColor = 0;
	int bestSum = -1;
	int r =  color        & 0xFF;
	int g = (color >>  8) & 0xFF;
	int b = (color >> 16) & 0xFF;
	for (int i = 0; i < 256; ++i) {
		int dr = r - src[2];
		int dg = g - src[1];
		int db = b - src[0];
		int sum = dr * dr + dg * dg + db * db;
		if (bestSum == -1 || sum < bestSum) {
			bestSum = sum;
			bestColor = i;
		}
		src += 4;
	}
	return bestColor;
}

void Game::redrawDialogueTexts() {
	debug(DBG_DIALOGUE, "Game::redrawDialogueTexts()");
	char *substringOffset[NUM_DIALOG_CHOICES][8];
	for (int i = 0; i < _dialogueChoiceCounter; ++i) {
		char *lastWord = 0;
		int lastStringLen = 0;
		int stringLen = 0;
		int substringCount = 0;
		for (char *p = _dialogueChoiceText[i]; *p; ++p) {
			int chr = (uint8_t)*p;
			if (_textCp949 && (chr & 0x80) != 0) {
				++p;
				stringLen += 16;
			} else {
				stringLen += _fontCharWidth[chr] + 1;
			}
			if (stringLen > _dialogueTextRect.w) {
				assert(substringCount < 8);
				substringOffset[i][substringCount] = lastWord;
				++substringCount;
				stringLen -= lastStringLen;
			}
			if (chr == ' ') {
				lastWord = p;
				lastStringLen = stringLen;
			}
		}
	}

	uint8_t *textBuffer = (uint8_t *)malloc(_dialogueTextRect.w * _dialogueTextRect.h);
	if (!textBuffer) {
		return;
	}

	memset(textBuffer, 0, _dialogueTextRect.w * _dialogueTextRect.h);
	int y = 0;
	for (int i = 0; i < _dialogueChoiceCounter; ++i) {
		int color = (_dialogueSpeechIndex == i) ? _selectedDialogueChoiceColor : _unselectedDialogueChoiceColor;
		uint8_t choiceColor = findBestMatchingColor(_bitmapBuffer0 + kOffsetBitmapPalette, color);
		int x = 0;
		int substring = 0;
		for (char *p = _dialogueChoiceText[i]; *p; ++p) {
			if (p == substringOffset[i][substring]) {
				++substring;
				y += 16;
				x = 0;
			} else {
				if (y + 16 >= _dialogueTextRect.h) {
					break;
				}
				int chr = (uint8_t)*p;
				if (_textCp949 && (chr & 0x80) != 0) {
					++p;
					chr = (chr << 8) | (uint8_t)*p;
					drawHangulGlyph(textBuffer + (_dialogueTextRect.h - 1 - y) * _dialogueTextRect.w + x, _dialogueTextRect.w, _hangulFontData, _hangulFontLutOffset, chr, choiceColor);
					x += 16;
				} else {
					drawChar(textBuffer + (_dialogueTextRect.h - 1 - y) * _dialogueTextRect.w + x, _dialogueTextRect.w, _fontData, chr, choiceColor);
					x += _fontCharWidth[chr] + 1;
				}
			}
		}
		y += 16;
	}
	_stub->copyRect(_dialogueTextRect.x, _dialogueTextRect.y, _dialogueTextRect.w, _dialogueTextRect.h, textBuffer, _dialogueTextRect.w, true);
	free(textBuffer);
}

void Game::initDialogue() {
	playMusic("..\\midi\\sadialog.mid");
	for (int spr = 0; spr < 3; ++spr) {
		for (int i = 0; i < 105; ++i) {
			_dialogueSpriteDataTable[spr][i] = 0;
		}
	}

	_dialogueFrameSpriteData = loadFile("..\\wgp\\frame.spr");
	loadDialogueSprite(0);
	loadDialogueSprite(1);
	loadDialogueSprite(2);
	loadDialogueData(_scriptDialogFileName);

	_stub->_pi.dirMask = 0;
	_stub->_pi.escape = false;
	_stub->_pi.enter = false;
}

void Game::handleDialogue() {
	debug(DBG_DIALOGUE, "Game::handleDialogue()");

		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (_dialogueChoiceSelected == 0 && _dialogueSpeechIndex < _dialogueChoiceCounter - 1) {
				++_dialogueSpeechIndex;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (_dialogueChoiceSelected == 0 && _dialogueSpeechIndex > 0) {
				--_dialogueSpeechIndex;
			}
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			_nextState = kStateGame;
			return;
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			if (_dialogueChoiceCounter > 1 && _dialogueChoiceSelected == 0) {
				win16_sndPlaySound(3, _dialogueChoiceSpeechSoundFile[_dialogueSpeechIndex]);
				_dialogueChoiceSelected = 1;
			} else {
				win16_sndPlaySound(6);
			}
		}
		if (_dialogueChoiceSelected != 0 && win16_sndPlaySound(22)) {
			_dialogueChoiceSelected = 0;
			strcpy(_tempTextBuffer, _dialogueChoiceNextId[_dialogueSpeechIndex]);
			if (_dialogueChoiceGotoFlag[_dialogueSpeechIndex]) {
				setupDialog(_dialogueChoiceNextId[_dialogueSpeechIndex]);
				if (_dialogueChoiceCounter == 0) {
					_nextState = kStateGame;
					return;
				}
			} else {
				int n = atoi(_tempTextBuffer);
				if (n == 100) { // special scene, Jack kisses Natalia
					_dialogueSpriteIndex = 2;
				} else {
					_dialogueEndedFlag = 1;
					_lastDialogueEndedId = atoi(_tempTextBuffer);
					return;
				}
			}
		}

		redrawDialogueBackground();
		redrawDialogueSprite(_dialogueSpriteIndex);
		if (_dialogueSpriteIndex == 2) {
			if (_dialogueSpriteCurrentFrameTable[2] == 0) {
				_dialogueEndedFlag = 1;
				return;
			}
		} else {
			redrawDialogueTexts();
		}
}

void Game::finiDialogue() {
	unloadDialogueData();
	if (_dialogueFrameSpriteData) {
		free(_dialogueFrameSpriteData);
		_dialogueFrameSpriteData = 0;
	}
	for (int spr = 0; spr < 3; ++spr) {
		for (int i = 0; i < 105; ++i) {
			free(_dialogueSpriteDataTable[spr][i]);
			_dialogueSpriteDataTable[spr][i] = 0;
		}
	}
	playMusic(_musicName);
	memset(_keysPressed, 0, sizeof(_keysPressed));
}

void Game::unloadDialogueData() {
	debug(DBG_DIALOGUE, "Game::unloadDialogueData() %d", _loadDialogueDataState);
	if (_loadDialogueDataState == 2) {
		free(_dialogueDescriptionBuffer);
		_dialogueDescriptionBuffer = 0;
		_loadDialogueDataState = 1;
	}
}

void Game::setupDialog(const char *dialogId) {
	debug(DBG_DIALOGUE, "Game::setupDialog('%s')", dialogId);
	if (dialogId[0] == 'J' || dialogId[0] == 'j') {
		_dialogueSpriteIndex = 0;
	} else {
		_dialogueSpriteIndex = 1;
	}
	_dialogueSpeechIndex = 0;
	_dialogueChoiceCounter = 0;
	for (int i = 0; i < _dialogueChoiceSize; ++i) {
		if (strcmp(_dialogueChoiceData[i].id, dialogId) == 0) {
			assert(_dialogueChoiceCounter < NUM_DIALOG_CHOICES);
			_dialogueChoiceGotoFlag[_dialogueChoiceCounter] = _dialogueChoiceData[i].gotoFlag;
			_dialogueChoiceNextId[_dialogueChoiceCounter] = _dialogueChoiceData[i].nextId;
			_dialogueChoiceSpeechSoundFile[_dialogueChoiceCounter] = _dialogueChoiceData[i].speechSoundFile;
			_dialogueChoiceText[_dialogueChoiceCounter] = _dialogueChoiceData[i].text;
			debug(DBG_DIALOGUE, "Add dialog choice goto %d nextId %s speechSoundFile %s text \"%s\"",
				_dialogueChoiceGotoFlag[_dialogueChoiceCounter],
				_dialogueChoiceNextId[_dialogueChoiceCounter],
				_dialogueChoiceSpeechSoundFile[_dialogueChoiceCounter],
				_dialogueChoiceText[_dialogueChoiceCounter]);
			++_dialogueChoiceCounter;
		}
	}
	// only one choice, play the speech directly
	if (_dialogueChoiceCounter == 1) {
		win16_sndPlaySound(3, _dialogueChoiceSpeechSoundFile[_dialogueSpeechIndex]);
		_dialogueChoiceSelected = 1;
	}
}

void Game::loadDialogueSprite(int spr) {
	debug(DBG_DIALOGUE, "Game::loadDialogueSprite(%d)", spr);
	const char *spriteFile = "..\\wgp\\kiss.spr";
	switch (spr) {
	case 0:
		spriteFile = _scriptDialogSprite1;
		break;
	case 1:
		spriteFile = _scriptDialogSprite2;
		break;
	}
	FileHolder fp(_fs, spriteFile);
	int tag = fp->readUint16LE();
	if (tag != 0x3553) {
		error("Invalid spr format %X", tag);
	}
	int count = fp->readUint16LE();
	assert(count <= 105);
	for (int i = 0; i < count; ++i) {
		int size = fp->readUint16LE();
		_dialogueSpriteDataTable[spr][i] = (uint8_t *)malloc(size + 10);
		if (_dialogueSpriteDataTable[spr][i]) {
			fp->read(_dialogueSpriteDataTable[spr][i], size + 10);
		}
	}
	_dialogueSpriteFrameCountTable[spr] = count;
	_dialogueSpriteCurrentFrameTable[spr] = 0;
}

void Game::loadDialogueData(const char *filename) {
	debug(DBG_DIALOGUE, "Game::loadDialogueData(%s)", filename);
	if (_loadDialogueDataState == 0) {
		_loadDialogueDataState = 1;
	} else {
		unloadDialogueData();
	}
	FileHolder fp(_fs, filename);
	_dialogueDescriptionSize = fp->size();
	_dialogueDescriptionBuffer = (char *)malloc(_dialogueDescriptionSize + 1);
	if (_dialogueDescriptionBuffer) {
		_loadDialogueDataState = 2;
		fp->read(_dialogueDescriptionBuffer, _dialogueDescriptionSize);
		_dialogueDescriptionBuffer[_dialogueDescriptionSize] = 0;
		stringStripComments(_dialogueDescriptionBuffer);
		parseDLG();
		setupDialog(_scriptDialogId);
	}
}

void Game::redrawDialogueSprite(int num) {
	debug(DBG_DIALOGUE, "Game::redrawDialogueSprite(%d)", num);
	decodeLzss(_dialogueFrameSpriteData + 2, _tempDecodeBuffer);
	uint8_t *spriteBitmap = _tempDecodeBuffer + 25000;
	decodeLzss(_dialogueSpriteDataTable[num][_dialogueSpriteCurrentFrameTable[num]], spriteBitmap);
	int sprY = getBitmapHeight(_tempDecodeBuffer) - getBitmapHeight(spriteBitmap);
	int sprX = getBitmapWidth(_tempDecodeBuffer) - getBitmapWidth(spriteBitmap);
	int frameX = 0;
	switch (num) {
	case 0:
		frameX = _dialogueBackgroundRect.x + 13;
		_dialogueTextRect.x = frameX + getBitmapWidth(_tempDecodeBuffer) + 10;
		break;
	case 1:
		frameX = _dialogueBackgroundRect.x + _dialogueBackgroundRect.w - getBitmapWidth(_tempDecodeBuffer) - 13;
		_dialogueTextRect.x = _dialogueBackgroundRect.x + 10;
		break;
	case 2:
		frameX = _dialogueBackgroundRect.x + (_dialogueBackgroundRect.w - getBitmapWidth(_tempDecodeBuffer)) / 2;
		_dialogueTextRect.x = _dialogueBackgroundRect.x;
		break;
	}
	_dialogueTextRect.w = _dialogueBackgroundRect.w - 10 - getBitmapWidth(_tempDecodeBuffer) - 13;

	int frameY = _dialogueBackgroundRect.y + (_dialogueBackgroundRect.h - getBitmapHeight(_tempDecodeBuffer)) / 2;
	_dialogueTextRect.y = _dialogueBackgroundRect.y + 10;
	_dialogueTextRect.h = _dialogueBackgroundRect.h - 20;

	_stub->copyRect(frameX, frameY, getBitmapWidth(_tempDecodeBuffer), getBitmapHeight(_tempDecodeBuffer), getBitmapData(_tempDecodeBuffer), getBitmapWidth(_tempDecodeBuffer));
	frameX += sprX / 2;
	frameY += sprY / 2;
	_stub->copyRect(frameX, frameY, getBitmapWidth(spriteBitmap), getBitmapHeight(spriteBitmap), getBitmapData(spriteBitmap), getBitmapWidth(spriteBitmap));
	++_dialogueSpriteCurrentFrameTable[num];
	if (_dialogueSpriteCurrentFrameTable[num] >= _dialogueSpriteFrameCountTable[num]) {
		_dialogueSpriteCurrentFrameTable[num] = 0;
	}
}

void Game::redrawDialogueBackground() {
	debug(DBG_DIALOGUE, "Game::redrawDialogueBackground()");
	sortObjects();
	int previousObject = -1;
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		SceneObject *so = _sortedSceneObjectsTable[i];
		if (so->statePrev == 1 || so->statePrev == 2) {
			if (previousObject >= 0) {
				redrawObjectBoxes(previousObject, i);
			}
			previousObject = i;
			decodeLzss(_sceneObjectFramesTable[so->frameNumPrev].data, _tempDecodeBuffer);
			SceneObjectFrame *sof = &_sceneObjectFramesTable[so->frameNumPrev];
			if (so->flipPrev == 2) {
				int y = _bitmapBuffer1.h + 1 - so->yPrev - sof->hdr.h;
				drawObjectVerticalFlip(so->xPrev, y, _tempDecodeBuffer, &_bitmapBuffer1);
			} else {
				int y = _bitmapBuffer1.h + 1 - so->yPrev - sof->hdr.h;
				drawObject(so->xPrev, y, _tempDecodeBuffer, &_bitmapBuffer1);
			}
		}
	}
	if (previousObject >= 0) {
		redrawObjectBoxes(previousObject, previousObject);
	}

	const uint8_t *src = _bitmapBuffer1.bits + _dialogueBackgroundRect.y * _bitmapBuffer1.pitch + _dialogueBackgroundRect.x;
	_stub->copyRect(_dialogueBackgroundRect.x, _dialogueBackgroundRect.y, _dialogueBackgroundRect.w, _dialogueBackgroundRect.h, src, _bitmapBuffer1.pitch);
	_stub->darkenRect(_dialogueBackgroundRect.x, _dialogueBackgroundRect.y, _dialogueBackgroundRect.w, _dialogueBackgroundRect.h);

	for (int i = 0; i < _sceneObjectsCount; ++i) {
		SceneObject *so = _sortedSceneObjectsTable[i];
		if (so->statePrev == 1) {
			SceneObjectFrame *sof = &_sceneObjectFramesTable[so->frameNumPrev];
			int y = _bitmapBuffer1.h + 1 - so->yPrev - sof->hdr.h;
			copyBufferToBuffer(so->xPrev, y, sof->hdr.w, sof->hdr.h, &_bitmapBuffer3, &_bitmapBuffer1);
		}
	}
}
