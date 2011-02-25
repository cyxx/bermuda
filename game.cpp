/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "avi_player.h"
#include "decoder.h"
#include "file.h"
#include "game.h"
#include "mixer.h"
#include "str.h"
#include "systemstub.h"

static const char *_gameWindowTitle = "Bermuda Syndrome";

Game::Game(SystemStub *stub, const char *dataPath, const char *savePath, const char *musicPath)
	: _fs(dataPath), _stub(stub), _dataPath(dataPath), _savePath(savePath), _musicPath(musicPath) {
	_mixer = new Mixer(_stub);
	_stateSlot = 1;
	detectVersion();
}

Game::~Game() {
	delete _mixer;
}

void Game::detectVersion() {
	// underscore ('_') for french & english versions and dash ('-') for german
	static const char *startupScenesTable[] = { "-01.SCN", "_01.SCN", 0 };
	_startupScene = 0;
	for (int i = 0; startupScenesTable[i]; ++i) {
		if (_fs.existFile(startupScenesTable[i])) {
			_startupScene = startupScenesTable[i];
			break;
		}
	}
	if (!_startupScene) {
		error("Unable to find startup scene file");
	}
	_isDemo = _fs.existFile("-00.SCN");
}

void Game::restart() {
	_mixer->stopAll();
	_mixerSoundId = Mixer::kDefaultSoundId;
	_mixerMusicId = Mixer::kDefaultSoundId;

	_lifeBarCurrentFrame = 0;
	_bagObjectAreaBlinkCounter = 0;
	_bagWeaponAreaBlinkCounter = 0;

	_lastDialogueEndedId = 0;
	_dialogueEndedFlag = 0;

	memset(_defaultVarsTable, 0, sizeof(_defaultVarsTable));
	memset(_varsTable, 0, sizeof(_varsTable));

	_scriptDialogId = 0;
	_scriptDialogFileName = 0;
	_scriptDialogSprite1 = 0;
	_scriptDialogSprite2 = 0;

	_switchScene = true;
	_startDialogue = false;
	_loadState = false;
	_clearSceneData = false;
	_gameOver = false;

	_loadDataState = 0;
	_previousBagAction = _currentBagAction = 0;
	_previousBagObject = _currentBagObject = -1;
	_currentPlayingSoundPriority = 0;
	_lifeBarDisplayed2 = _lifeBarDisplayed = false;

	memset(_keysPressed, 0, sizeof(_keysPressed));
	_musicTrack = 0;
	_musicName[0] = 0;
	_sceneNumber = 0;
	_currentSceneWgp[0] = 0;
	_tempTextBuffer[0] = 0;
	_currentSceneScn[0] = 0;

	_bagPosX = 585;
	_bagPosY = 23;

	memset(_sortedSceneObjectsTable, 0, sizeof(_sortedSceneObjectsTable));
	memset(_sceneObjectsTable, 0, sizeof(_sceneObjectsTable));
	_sceneObjectsCount = 0;
	memset(_animationsTable, 0, sizeof(_animationsTable));
	_animationsCount = 0;
	memset(_soundBuffersTable, 0, sizeof(_soundBuffersTable));
	_soundBuffersCount = 0;
	memset(_boxesTable, 0, sizeof(_boxesTable));
	memset(_boxesCountTable, 0, sizeof(_boxesCountTable));
	memset(_sceneObjectFramesTable, 0, sizeof(_sceneObjectFramesTable));
	_sceneObjectFramesCount = 0;
	memset(_bagObjectsTable, 0, sizeof(_bagObjectsTable));
	_bagObjectsCount = 0;
	memset(_sceneObjectMotionsTable, 0, sizeof(_sceneObjectMotionsTable));
	_sceneObjectMotionsCount = 0;
	memset(_nextScenesTable, 0, sizeof(_nextScenesTable));
	_sceneConditionsCount = 0;
	memset(_sceneObjectStatusTable, 0, sizeof(_sceneObjectStatusTable));
	_sceneObjectStatusCount = 0;

	strcpy(_tempTextBuffer, _startupScene);
}

void Game::mainLoop() {
	_stub->init(_gameWindowTitle, kGameScreenWidth, kGameScreenHeight);
	allocateTables();
	loadCommonSprites();
	_mixer->open();
	restart();
	if (_isDemo) {
		playBitmapSequenceDemo();
	} else {
		playVideo("DATA/LOGO.AVI");
		playVideo("DATA/INTRO.AVI");
	}
	_lastFrameTimeStamp = _stub->getTimeStamp();
	while (!_stub->_quit) {
		if (_switchScene) {
			_switchScene = false;
			if (stringEndsWith(_tempTextBuffer, "SCN")) {
				win16_sndPlaySound(6);
				debug(DBG_GAME, "switch to scene '%s'", _tempTextBuffer);
				if (strcmp(_tempTextBuffer, "PIC4.SCN") == 0) {
					debug(DBG_INFO, "End of game");
					break;
				}
				strcpy(_currentSceneScn, _tempTextBuffer);
				parseSCN(_tempTextBuffer);
			} else if (stringEndsWith(_tempTextBuffer, "SAV")) {
				if (_isDemo && strcmp(_tempTextBuffer, "A16.SAV") == 0) {
					debug(DBG_GAME, "End of demo interactive part");
					restart();
					continue;
				}
				warning("Ignoring savestate load '%s'", _tempTextBuffer);
				// should work though, as the original savestates load fine
				// now, but this "feature" is only used in the demo AFAICT,
				// so no need to bother...
			} else {
				debug(DBG_GAME, "load mov '%s'", _tempTextBuffer);
				loadMOV(_tempTextBuffer);
			}
			if (_loadState) {
				_loadState = false;
				loadState(_stateSlot, false);
				playMusic(_musicName);
				memset(_keysPressed, 0, sizeof(_keysPressed));
			}
			assert(_sceneObjectsCount != 0);
			if (_currentBagObject == -1) {
				_currentBagObject = _bagObjectsCount - 1;
				if (_currentBagObject > 0) {
					_currentBagObject = 0;
				}
			}
			if (_loadDataState != 0) {
				setupScreenPalette(_bitmapBuffer0 + kOffsetBitmapPalette);
			}
			_gameOver = false;
			_workaroundRaftFlySceneBug = strncmp(_currentSceneScn, "FLY", 3) == 0;
		}
		updateKeysPressedTable();
		updateMouseButtonsPressed();
		runObjectsScript();
		if (!_switchScene) {
			_stub->updateScreen();
			uint32 end = _lastFrameTimeStamp + kCycleDelay;
			do {
				_stub->sleep(10);
				_stub->processEvents();
			} while (!_stub->_pi.fastMode && _stub->getTimeStamp() < end);
			_lastFrameTimeStamp = _stub->getTimeStamp();
		}
		if (_startDialogue) {
			_startDialogue = false;
			handleDialogue();
		}
	}
	clearSceneData(-1);
	deallocateTables();
	unloadCommonSprites();
	_mixer->close();
	_stub->destroy();
}

void Game::updateMouseButtonsPressed() {
	_mouseButtonsPressed = 0;
	if (_stub->_pi.leftMouseButton) {
		_stub->_pi.leftMouseButton = false;
		_mouseButtonsPressed |= kLeftMouseButton;
	}
	if (_stub->_pi.rightMouseButton) {
		_stub->_pi.rightMouseButton = false;
		_mouseButtonsPressed |= kRightMouseButton;
	}
}

void Game::updateKeysPressedTable() {
	debug(DBG_GAME, "Game::updateKeysPressedTable()");
	_keysPressed[13] = _stub->_pi.enter ? 1 : 0;
	_keysPressed[16] = _stub->_pi.shift ? 1 : 0;
	_keysPressed[32] = _stub->_pi.space ? 1 : 0;
	_keysPressed[37] = (_stub->_pi.dirMask & PlayerInput::DIR_LEFT)  ? 1 : 0;
	_keysPressed[38] = (_stub->_pi.dirMask & PlayerInput::DIR_UP)    ? 1 : 0;
	_keysPressed[39] = (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) ? 1 : 0;
	_keysPressed[40] = (_stub->_pi.dirMask & PlayerInput::DIR_DOWN)  ? 1 : 0;
	if (_stub->_pi.tab) {
		_stub->_pi.tab = false;
		handleBagMenu();
	}
	if (_stub->_pi.ctrl) {
		_stub->_pi.ctrl = false;
		_lifeBarDisplayed = !_lifeBarDisplayed;
	}
	if (_stub->_pi.stateSlot != 0) {
		int slot = _stateSlot + _stub->_pi.stateSlot;
		if (slot >= 1 && slot <= 999) {
			_stateSlot = slot;
			debug(DBG_INFO, "Current game state slot is %d", _stateSlot);
		}
		_stub->_pi.stateSlot = 0;
	}
	if (_stub->_pi.load) {
		_stub->_pi.load = false;
		loadState(_stateSlot, true);
		_loadState = _switchScene; // gamestate will get loaded on scene switch
	}
	if (_stub->_pi.save) {
		_stub->_pi.save = false;
		saveState(_stateSlot);
	}
	if (_gameOver) {
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			restart();
		}
	}
}

void Game::setupScreenPalette(const uint8 *src) {
	_stub->setPalette(src, 256);
}

void Game::clearSceneData(int anim) {
	debug(DBG_GAME, "Game::clearSceneData(%d)", anim);
	if (anim == -1) {
		_sceneConditionsCount = 0;
		_soundBuffersCount = 0;
		_animationsCount = 0;
		_sceneObjectsCount = 0;
		_sceneObjectMotionsCount = 0;
		_sceneObjectFramesCount = 0;
		_loadDataState = 0;
	} else {
		_animationsCount = anim + 1;
		SceneAnimation *sa = &_animationsTable[anim];
		_sceneObjectMotionsCount = sa->firstMotionIndex + sa->motionsCount;
		SceneObjectMotion *som = &_sceneObjectMotionsTable[_sceneObjectMotionsCount - 1];
		_sceneObjectFramesCount = som->firstFrameIndex + som->count;
		_sceneObjectsCount = sa->firstObjectIndex + sa->objectsCount;
		_soundBuffersCount = sa->firstSoundBufferIndex + sa->soundBuffersCount;
		_sceneConditionsCount = 0;
		_loadDataState = 2;
	}
	win16_sndPlaySound(7);
//	for (int i = NUM_SOUND_BUFFERS - 1; i >= _soundBuffersCount; --i) {
//		SoundBuffer *sb = &_soundBuffersTable[i];
//		if (sb->buffer) {
//			free(sb->buffer);
//			sb->buffer = 0;
//		}
//	}
//	for (int i = NUM_SCENE_OBJECT_FRAMES - 1; i >= _sceneObjectFramesCount; --i) {
	for (int i = _sceneObjectFramesCount; i < NUM_SCENE_OBJECT_FRAMES; ++i) {
		SceneObjectFrame *sof = &_sceneObjectFramesTable[i];
		if (sof->data) {
			free(sof->data);
			sof->data = 0;
		}
	}
//	for (int i = NUM_SCENE_ANIMATIONS - 1; i >= _animationsCount; --i) {
	for (int i = _animationsCount; i < NUM_SCENE_ANIMATIONS; ++i) {
		SceneAnimation *sa = &_animationsTable[i];
		if (sa->scriptData) {
			free(sa->scriptData);
			sa->scriptData = 0;
		}
	}
//	for (int i = NUM_SCENE_OBJECTS - 1; i >= _sceneObjectsCount; --i) {
	for (int i = _sceneObjectsCount; i < NUM_SCENE_OBJECTS; ++i) {
		SceneObject *so = &_sceneObjectsTable[i];
		so->state = 0;
		memset(so->varsTable, 0, sizeof(so->varsTable));
	}
	for (int i = 0; i < 10; ++i) {
		_boxesCountTable[i] = 0;
	}
}

void Game::reinitializeObject(int object) {
	debug(DBG_GAME, "Game::reinitializeObject(%d)", object);
	SceneObject *so = derefSceneObject(object);
	if (so->state != 1 && so->state != 2) {
		int16 state = 0;
		switch (so->mode) {
		case 1:
			state = 1;
			break;
		case 2: {
				int16 rnd = _rnd.getNumber();
				int t = (rnd * so->modeRndMul) / 0x8000;
				if ((t & 0xFFFF) == 0) {
					state = 1;
				}
			}
			break;
		case 3:
			state = 2;
			break;
		}
		if (state != 0) {
			so->x = so->xInit;
			so->y = so->yInit;
			so->zPrev = so->z = so->zInit;
			so->flipPrev = so->flip = so->flipInit;
			so->motionNum1 = so->motionNum2 = so->motionNum + so->motionInit;
			so->frameNum = _sceneObjectMotionsTable[so->motionNum2].firstFrameIndex + so->motionFrameNum;
			if (so->flip == 2) {
				so->x -= _sceneObjectFramesTable[so->frameNum].hdr.w - 1;
			}
			if (so->flip == 1) {
				so->y -= _sceneObjectFramesTable[so->frameNum].hdr.h - 1;
			}
			if (so->state == 0) {
				so->xPrev = so->x;
				so->yPrev = so->y;
				so->frameNumPrev = so->frameNum;
			}
			so->state = so->statePrev = state;
		}
	}
}

void Game::updateObjects() {
	debug(DBG_GAME, "Game::updateObjects()");
	redrawObjects();
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		SceneObject *so = &_sceneObjectsTable[i];
		if (so->state == -1) {
			so->state = 0;
		}
	}
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		SceneObject *so = &_sceneObjectsTable[i];
		so->statePrev = so->state;
		if (so->state == 1) {
			so->motionNum1 = so->motionNum2;
			so->flipPrev = so->flip;
			so->zPrev = so->z;
			so->xPrev = so->x;
			so->yPrev = so->y;
			so->frameNumPrev = so->frameNum;
			assert(so->frameNumPrev >= 0 && so->frameNumPrev < _sceneObjectFramesCount);
			assert(so->frameNum >= 0 && so->frameNum < _sceneObjectFramesCount);
			assert(so->motionNum2 >= 0 && so->motionNum2 < _sceneObjectMotionsCount);
			so->frameNum = _sceneObjectFramesTable[so->frameNumPrev].hdr.num + _sceneObjectMotionsTable[so->motionNum2].firstFrameIndex;
			const int dx = _sceneObjectFramesTable[so->frameNumPrev].hdr.xPos - _sceneObjectFramesTable[so->frameNum].hdr.xPos;
			const int dy = _sceneObjectFramesTable[so->frameNumPrev].hdr.yPos - _sceneObjectFramesTable[so->frameNum].hdr.yPos;
			if (so->flip == 2) {
				int ds = _sceneObjectFramesTable[so->frameNumPrev].hdr.w - _sceneObjectFramesTable[so->frameNum].hdr.w;
				so->x += dx + ds;
			} else {
				so->x -= dx;
			}
			so->y -= dy;
		}
	}
}

void Game::runObjectsScript() {
	debug(DBG_GAME, "Game::runObjectsScript()");
	_objectScript.nextScene = -1;
	assert(_loadDataState != 3); // unneeded code
	if (_varsTable[309]) {
		memset(_keysPressed, 0, sizeof(_keysPressed));
	}
	if (_loadDataState == 2) {
		int start = _workaroundRaftFlySceneBug ? 1 : 0;
		_workaroundRaftFlySceneBug = false;
		for (int i = start; i < _sceneObjectsCount; ++i) {
			SceneObject *so = &_sceneObjectsTable[i];
			if (so->statePrev == 0 || so->statePrev == -1) {
				continue;
			}
			debug(DBG_GAME, "Game::runObjectsScript() currentObjectNum=%d", i);
			_objectScript.currentObjectNum = i;
			int anim = _sceneObjectMotionsTable[so->motionNum1].animNum;
			assert(anim >= 0 && anim < _animationsCount);
			_objectScript.data = _animationsTable[anim].scriptData;
			int endOfDataOffset = _animationsTable[anim].scriptSize; // endOfDataOffset2
			_objectScript.dataOffset = 0;
			int statement = 0;
			while (_objectScript.dataOffset < endOfDataOffset) {
				_objectScript.statementNum = statement;
				int endOfStatementDataOffset = _objectScript.fetchNextWord(); // endOfDataOffset1
				_objectScript.testObjectNum = -1;
				_objectScript.testDataOffset = endOfStatementDataOffset;
				bool loop = true;
				while (loop) {
					int op = _objectScript.fetchNextWord();
					debug(DBG_OPCODES, "statement %d condition %d op %d", statement, i, op);
					if (op == 0) {
						break;
					}
					const GameConditionOpcode *cop = findConditionOpcode(op);
					if (!cop) {
						error("Invalid condition %d", op);
					}
					loop = (this->*(cop->pf))();
				}
				if (loop) {
					while (_objectScript.dataOffset < endOfStatementDataOffset) {
						int op = _objectScript.fetchNextWord();
						debug(DBG_OPCODES, "statement %d operator %d op %d", statement, i, op);
						if (op == 100) { // &Game::oop_breakObjectScript
							endOfStatementDataOffset = _objectScript.dataOffset = endOfDataOffset;
							break;
						}
						const GameOperatorOpcode *oop = findOperatorOpcode(op);
						if (!oop) {
							error("Invalid operator %d", op);
						}
						(this->*(oop->pf))();
					}
				}
				_objectScript.dataOffset = endOfStatementDataOffset;
				++statement;
			}
		}
		_dialogueEndedFlag = 0;
		if (_objectScript.nextScene != -1 && _objectScript.nextScene < _sceneConditionsCount) {
			strcpy(_tempTextBuffer, _nextScenesTable[_objectScript.nextScene].name);
			_switchScene = true;
		}
		for (int i = 0; i < _sceneObjectsCount; ++i) {
			reinitializeObject(i);
		}
#if 0 // cheat: no hit
		_varsTable[0] = 0;
#endif
		if (_varsTable[0] >= 10 && !_gameOver) {
			strcpy(_musicName, "..\\midi\\gameover.mid");
			playMusic(_musicName);
			_gameOver = true;
//			_skipUpdateScreen = false;
		}
		if (_loadDataState == 2) {
			updateObjects();
		}
	}
//	_skipUpdateScreen = false;
	if (_varsTable[241] == 1) {
//		_startEndingScene = true;
		stopMusic();
		clearSceneData(-1);
		_varsTable[241] = 2;
		playVideo("DATA/FINAL.AVI");
		strcpy(_tempTextBuffer, "END.SCN");
		_switchScene = true;
	}
}

int Game::findBagObjectByName(const char *objectName) const {
	debug(DBG_GAME, "Game::findBagObjectByName()", objectName);
	int index = -1;
	for (int i = 0; i < _bagObjectsCount; ++i) {
		if (strcasecmp(_bagObjectsTable[i].name, objectName) == 0) {
			index = i;
			break;
		}
	}
	return index;
}

int Game::getObjectTranslateXPos(int object, int dx1, int div, int dx2) {
	debug(DBG_GAME, "Game::getObjectTranslateXPos(%d, %d, %d, %d)", object, dx1, div, dx2);
	SceneObject *so = derefSceneObject(object);
	int16 _di = _sceneObjectMotionsTable[so->motionNum + so->motionInit].firstFrameIndex + so->motionFrameNum;
	int16 _ax, _dx;
	if (so->flip == 2) {
		_ax = _sceneObjectFramesTable[_di].hdr.xPos - _sceneObjectFramesTable[so->frameNum].hdr.xPos;
		_ax += _sceneObjectFramesTable[_di].hdr.w - _sceneObjectFramesTable[so->frameNum].hdr.w;
		_ax += dx1;
	} else {
		_ax = _sceneObjectFramesTable[so->frameNum].hdr.xPos - _sceneObjectFramesTable[_di].hdr.xPos;
	}
	if (so->flipInit == 2) {
		_dx = 1 - _sceneObjectFramesTable[_di].hdr.w - dx1;
	} else {
		_dx = 0;
	}
	_ax = so->x - so->xInit - _dx - _ax - dx2;
	int16 _si = _ax % div;
	if (_si < 0) {
		_si += div;
	}
	return _si;
}

int Game::getObjectTranslateYPos(int object, int dy1, int div, int dy2) {
	debug(DBG_GAME, "Game::getObjectTranslateYPos(%d, %d, %d, %d)", object, dy1, div, dy2);
	SceneObject *so = derefSceneObject(object);
	int16 _di = _sceneObjectMotionsTable[so->motionNum + so->motionInit].firstFrameIndex + so->motionFrameNum;
	int16 _ax, _dx;
	if (so->flip == 1) {
		_ax = _sceneObjectFramesTable[_di].hdr.yPos - _sceneObjectFramesTable[so->frameNum].hdr.yPos;
		_ax += _sceneObjectFramesTable[_di].hdr.h - _sceneObjectFramesTable[so->frameNum].hdr.h;
		_ax += dy1;
	} else {
		_ax = _sceneObjectFramesTable[so->frameNum].hdr.yPos - _sceneObjectFramesTable[_di].hdr.yPos;
	}
	if (so->flipInit == 1) {
		_dx = 1 - _sceneObjectFramesTable[_di].hdr.h - dy1;
	} else {
		_dx = 0;
	}
	_ax = so->y - so->yInit - _dx - _ax - dy2;
	int16 _si = _ax % div;
	if (_si < 0) {
		_si += div;
	}
	return _si;
}

int Game::findObjectByName(int currentObjectNum, int defaultObjectNum, bool *objectFlag) {
	int index = -1;
	*objectFlag = true;
	int16 len = _objectScript.fetchNextWord();
	debug(DBG_GAME, "Game::findObjectByName() len = %d", len);
	if (len == -1) {
		index = defaultObjectNum;
	} else if (len == 0) {
		*objectFlag = false;
		index = currentObjectNum;
	} else {
		debug(DBG_GAME, "Game::findObjectByName() name = '%s' len = %d", _objectScript.getString(), len);
		for (int i = 0; i < _sceneObjectsCount; ++i) {
			if (strcmp(_sceneObjectsTable[i].name, _objectScript.getString()) == 0) {
				index = i;
				break;
			}
		}
		_objectScript.dataOffset += len;
	}
	return index;
}

void Game::sortObjects() {
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		_sortedSceneObjectsTable[i] = &_sceneObjectsTable[i];
	}
	for (int i = _sceneObjectsCount / 2; i > 0; i /= 2) {
		for (int j = i; j < _sceneObjectsCount; ++j) {
			for (int k = j - i; k >= 0; k -= i) {
				if (_sortedSceneObjectsTable[k]->z >= _sortedSceneObjectsTable[k + i]->z) {
					break;
				}
				SWAP(_sortedSceneObjectsTable[k], _sortedSceneObjectsTable[k + i]);
			}
		}
	}
}

void Game::copyBufferToBuffer(int x, int y, int w, int h, SceneBitmap *src, SceneBitmap *dst) {
	const uint8 *p_src = src->bits;

	const int x2 = MIN(src->w, dst->w);
	if (w <= 0 || x > x2 || x + w <= 0) {
		return;
	}
	if (x < 0) {
		w += x;
		p_src -= x;
		x = 0;
	}
	if (x + w > x2) {
		w = x2 + 1 - x;
	}

	const int y2 = MIN(src->h, dst->h);
	if (h <= 0 || y >= y2 || y + h <= 0) {
		return;
	}
	if (y < 0) {
		h += y;
		p_src -= y * src->pitch;
		y = 0;
	}
	if (y + h > y2) {
		h = y2 + 1 - y;
	}

	p_src += y * src->pitch + x;
	uint8 *p_dst = dst->bits + y * dst->pitch + x;
	while (h--) {
		memcpy(p_dst, p_src, w);
		p_dst += dst->pitch;
		p_src += src->pitch;
	}
}

void Game::drawBox(int x, int y, int w, int h, SceneBitmap *src, SceneBitmap *dst, int startColor, int endColor) {
	const uint8 *p_src = src->bits;

	const int x2 = MIN(src->w, dst->w);
	if (w <= 0 || x > x2 || x + w <= 0) {
		return;
	}
	if (x < 0) {
		w += x;
		p_src -= x;
		x = 0;
	}
	if (x + w > x2) {
		w = x2 + 1 - x;
	}

	const int y2 = MIN(src->h, dst->h);
	if (h <= 0 || y >= y2 || y + h <= 0) {
		return;
	}
	if (y < 0) {
		h += y;
		p_src -= y * src->pitch;
		y = 0;
	}
	if (y + h > y2) {
		h = y2 + 1 - y;
	}

	p_src += y * src->pitch + x;
	uint8 *p_dst = dst->bits + y * dst->pitch + x;
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (startColor > p_src[i] || endColor <= p_src[i]) {
				p_dst[i] = p_src[i];
			}
		}
		p_src += src->pitch;
		p_dst += dst->pitch;
	}
}

void Game::drawObject(int x, int y, const uint8 *src, SceneBitmap *dst) {
	int w = READ_LE_UINT16(src) + 1; src += 2;
	int h = READ_LE_UINT16(src) + 1; src += 2;

	int clippedW = w;
	if (x > dst->w || x + clippedW <= 0) {
		return;
	}
	if (x < 0) {
		clippedW += x;
		src -= x;
		x = 0;
	}
	if (x + clippedW > dst->w) {
		clippedW = dst->w + 1 - x;
	}

	int clippedH = h;
	if (y > dst->h || y + clippedH <= 0) {
		return;
	}
	if (y < 0) {
		clippedH += y;
		src -= y * w;
		y = 0;
	}
	if (y + clippedH > dst->h) {
		clippedH = dst->h + 1 - y;
	}

	for (int j = 0; j < clippedH; ++j) {
		for (int i = 0; i < clippedW; ++i) {
			if (src[i]) {
				dst->bits[dst->pitch * (y + j) + (x + i)] = src[i];
			}
		}
		src += w;
	}
}

void Game::drawObjectVerticalFlip(int x, int y, const uint8 *src, SceneBitmap *dst) {
	int w = READ_LE_UINT16(src) + 1; src += 2;
	int h = READ_LE_UINT16(src) + 1; src += 2;

	src += w - 1;

	int clippedW = w;
	if (x > dst->w || x + clippedW <= 0) {
		return;
	}
	if (x < 0) {
		clippedW += x;
		src += x;
		x = 0;
	}
	if (x + clippedW > dst->w) {
		clippedW = dst->w + 1 - x;
	}

	int clippedH = h;
	if (y > dst->h || y + clippedH <= 0) {
		return;
	}
	if (y < 0) {
		clippedH += y;
		src -= y * w;
		y = 0;
	}
	if (y + clippedH > dst->h) {
		clippedH = dst->h + 1 - y;
	}

	for (int j = 0; j < clippedH; ++j) {
		for (int i = 0; i < clippedW; ++i) {
			if (src[-i]) {
				dst->bits[dst->pitch * (y + j) + (x + i)] = src[-i];
			}
		}
		src += w;
	}
}

void Game::redrawObjectBoxes(int previousObject, int currentObject) {
	debug(DBG_GAME, "Game::redrawObjectBoxes(%d, %d)", previousObject, currentObject);
	for (int b = 0; b < 10; ++b) {
		for (int i = 0; i < _boxesCountTable[b]; ++i) {
			Box *box = derefBox(b, i);
			if (box->state == 2 && box->z <= _sortedSceneObjectsTable[previousObject]->z) {
				const int w = box->x2 - box->x1 + 1;
				const int h = box->y2 - box->y1 + 1;
				const int x = box->x1;
				const int y = _bitmapBuffer1.h + 1 - box->y2;
				if (previousObject == currentObject || box->z > _sortedSceneObjectsTable[currentObject]->z) {
					if (box->endColor != 0) {
						drawBox(x, y, w, h, &_bitmapBuffer3, &_bitmapBuffer1, box->startColor, box->startColor + box->endColor - 1);
					} else {
						copyBufferToBuffer(x, y, w, h, &_bitmapBuffer3, &_bitmapBuffer1);
					}
				}
			}
		}
	}
}

void Game::redrawObjects() {
	sortObjects();
	int previousObject = -1;
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		SceneObject *so = _sortedSceneObjectsTable[i];
		if (so->state == 1 || so->state == 2) {
			if (previousObject >= 0) {
				redrawObjectBoxes(previousObject, i);
			}
			previousObject = i;
			decodeLzss(_sceneObjectFramesTable[so->frameNum].data, _tempDecodeBuffer);
			if (_isDemo && _sceneNumber == 1 && i == 14) {
				// FIXME fixes wrong overlapping icon in the first scene of the demo
				//   object 13 pos 582,423 frame 1885 - should be displayed
				//   object 14 pos 582,423 frame 1884 - shouldn't be displayed
				continue;
			}
			if (so->flip == 2) {
				int16 y = _bitmapBuffer1.h + 1 - so->y - _sceneObjectFramesTable[so->frameNum].hdr.h;
				drawObjectVerticalFlip(so->x, y, _tempDecodeBuffer, &_bitmapBuffer1);
			} else {
				int16 y = _bitmapBuffer1.h + 1 - so->y - _sceneObjectFramesTable[so->frameNum].hdr.h;
				drawObject(so->x, y, _tempDecodeBuffer, &_bitmapBuffer1);
			}
		}
	}
	if (previousObject >= 0) {
		redrawObjectBoxes(previousObject, previousObject);
	}
	if (_sceneNumber != -1000 && _sceneObjectsCount != 0) {
		if (!_isDemo && _gameOver) {
			decodeLzss(_bermudaOvrData + 2, _tempDecodeBuffer);
			drawObject(93, _bitmapBuffer1.h - 230, _tempDecodeBuffer, &_bitmapBuffer1);
		}
		if (_currentBagObject >= 0 && _currentBagObject < _bagObjectsCount && _currentBagAction == 3) {
			drawObject(_bagPosX, _bitmapBuffer1.h + 1 - _bagPosY - getBitmapHeight(_iconBackgroundImage), _iconBackgroundImage, &_bitmapBuffer1);
			int invW = getBitmapWidth(_iconBackgroundImage);
			int invH = getBitmapHeight(_iconBackgroundImage);
			int bagObjW = getBitmapWidth(_bagObjectsTable[_currentBagObject].data);
			int bagObjH = getBitmapHeight(_bagObjectsTable[_currentBagObject].data);
			int y = _bitmapBuffer1.h + 1 - _bagPosY - (invH - bagObjH) / 2 - bagObjH;
			int x = _bagPosX + (invW - bagObjW) / 2;
			drawObject(x, y, _bagObjectsTable[_currentBagObject].data, &_bitmapBuffer1);
		}
		if (_lifeBarDisplayed) {
			drawObject(386, _bitmapBuffer1.h - 18 - getBitmapHeight(_lifeBarImage), _lifeBarImage, &_bitmapBuffer1);
			if (_varsTable[1] == 1) {
				drawObject(150, _bitmapBuffer1.h - 18 - getBitmapHeight(_lifeBarImage), _lifeBarImage, &_bitmapBuffer1);
				drawObject(173, _bitmapBuffer1.h - 18 - getBitmapHeight(_swordIconImage), _swordIconImage, &_bitmapBuffer1);
			} else if (_varsTable[2] == 1) {
				drawObject(150, _bitmapBuffer1.h - 18 - getBitmapHeight(_lifeBarImage), _lifeBarImage, &_bitmapBuffer1);
				int index = MIN(13, 13 - _varsTable[4]);
				drawObject(173, _bitmapBuffer1.h - 31 - getBitmapHeight(_weaponIconImageTable[index]), _weaponIconImageTable[index], &_bitmapBuffer1);
				if (_varsTable[3] < 5) {
					index = (_varsTable[4] <= 0) ? 0 : 1;
					uint8 *p = _ammoIconImageTable[index][_varsTable[3]];
					drawObject(184, _bitmapBuffer1.h - 41 - getBitmapHeight(p), p, &_bitmapBuffer1);
				}
			}
			int index = (_varsTable[0] >= 10) ? 10 : _varsTable[0];
			uint8 *lifeBarFrame = _lifeBarImageTable[index][_lifeBarCurrentFrame];
			drawObject(409, _bitmapBuffer1.h - 36 - getBitmapHeight(lifeBarFrame), lifeBarFrame, &_bitmapBuffer1);
			++_lifeBarCurrentFrame;
			if (_lifeBarCurrentFrame >= 12) {
				_lifeBarCurrentFrame = 0;
			}
		}
#if 0
		if (_lifeBarDisplayed || _lifeBarDisplayed2) {
			if (_varsTable[2] == 1 || _varsTable[1] == 1) {
				win16_stretchBits(&_bitmapBuffer1,
					getBitmapHeight(_lifeBarImage),
					getBitmapWidth(_lifeBarImage),
					_bitmapBuffer1.h - 18 - getBitmapHeight(_lifeBarImage),
					150,
					getBitmapHeight(_lifeBarImage),
					getBitmapWidth(_lifeBarImage),
					19,
					150
				);
			}
			win16_stretchBits(&_bitmapBuffer1,
				getBitmapHeight(_lifeBarImage),
				getBitmapWidth(_lifeBarImage),
				_bitmapBuffer1.h - 18 - getBitmapHeight(_lifeBarImage),
				386,
				getBitmapHeight(_lifeBarImage),
				getBitmapWidth(_lifeBarImage),
				19,
				386
			);
			_lifeBarDisplayed2 = _lifeBarDisplayed;
		}
		if (_previousBagAction == kActionUseObject || _currentBagAction == kActionUseObject) {
			if (_currentBagObject != _previousBagObject || _previousBagAction != _currentBagAction) {
				win16_stretchBits(&_bitmapBuffer1,
					getBitmapHeight(_iconBackgroundImage),
					getBitmapWidth(_iconBackgroundImage),
					_bitmapBuffer1.h + 1 - _bagPosY - getBitmapHeight(_iconBackgroundImage),
					_bagPosX,
					getBitmapHeight(_iconBackgroundImage),
					getBitmapWidth(_iconBackgroundImage),
					_bagPosY,
					_bagPosX
				);
			}
		}
#endif
	}
	win16_stretchBits(&_bitmapBuffer1, _bitmapBuffer1.h + 1, _bitmapBuffer1.w + 1, 0, 0, _bitmapBuffer1.h + 1, _bitmapBuffer1.w + 1, 0, 0);
	memcpy(_bitmapBuffer1.bits, _bitmapBuffer3.bits, kGameScreenWidth * kGameScreenHeight);
	if (_lifeBarDisplayed) {
		copyBufferToBuffer(386,
			_bitmapBuffer1.h + 1 - 19 - getBitmapHeight(_lifeBarImage),
			getBitmapWidth(_lifeBarImage),
			getBitmapHeight(_lifeBarImage),
			&_bitmapBuffer3,
			&_bitmapBuffer1
		);
		copyBufferToBuffer(150,
			_bitmapBuffer1.h + 1 - 19 - getBitmapHeight(_lifeBarImage),
			getBitmapWidth(_lifeBarImage),
			getBitmapHeight(_lifeBarImage),
			&_bitmapBuffer3,
			&_bitmapBuffer1
		);
	}
	_previousBagAction = _currentBagAction;
}

void Game::playVideo(const char *name) {
	char *filePath = (char *)malloc(strlen(_dataPath) + 1 + strlen(name) + 1);
	if (filePath) {
		sprintf(filePath, "%s/%s", _dataPath, name);
		File f;
		if (f.open(filePath)) {
			_stub->fillRect(0, 0, kGameScreenWidth, kGameScreenHeight, 0);
			_stub->updateScreen();
			_mixer->close();
			AVI_Player player(_stub);
			player.play(&f);
			_mixer->open();
		}
		free(filePath);
	}
}

void Game::playBitmapSequenceDemo() {
	const char *bitmapList[] = { "..\\wgp\\title.bmp", "..\\wgp\\title1.bmp", "..\\wgp\\title2.bmp" };
	for (int i = 0; i < 3; ++i) {
		loadWGP(bitmapList[i]);
		_stub->setPalette(_bitmapBuffer0 + kOffsetBitmapPalette, 256);
		_stub->copyRect(0, 0, kGameScreenWidth, kGameScreenHeight, _bitmapBuffer1.bits, _bitmapBuffer1.pitch);
		_stub->updateScreen();
		do {
			_stub->sleep(10);
			_stub->processEvents();
			if (_stub->_quit) {
				return;
			}
		} while (!_stub->_pi.enter);
		_stub->_pi.enter = false;
	}
}

void Game::stopMusic() {
	debug(DBG_GAME, "Game::stopMusic()");
	_mixer->stopSound(_mixerMusicId);
}

void Game::playMusic(const char *name) {
	static const struct {
		const char *fileName;
		int digitalTrack;
	} _midiMapping[] = {
		// retail game version
		{ "..\\midi\\flyaway.mid", 2 },
		{ "..\\midi\\jungle1.mid", 3 },
		{ "..\\midi\\sadialog.mid", 4 },
		{ "..\\midi\\caves.mid", 5 },
		{ "..\\midi\\jungle2.mid", 6 },
		{ "..\\midi\\darkcave.mid", 7 },
		{ "..\\midi\\waterdiv.mid", 8 },
		{ "..\\midi\\merian1.mid", 9 },
		{ "..\\midi\\telquad.mid", 10 },
		{ "..\\midi\\gameover.mid", 11 },
		{ "..\\midi\\complete.mid", 12 },
		// demo game version
		{ "..\\midi\\musik.mid", 3 }
	};
	assert(_musicTrack == 0);
	if (name[0] == 0) {
		return;
	}
	debug(DBG_GAME, "Game::playMusic('%s')", name);
	stopMusic();
	for (unsigned int i = 0; i < ARRAYSIZE(_midiMapping); ++i) {
		if (strcasecmp(_midiMapping[i].fileName, name) == 0) {
			char filePath[512];
			sprintf(filePath, "%s/track%02d.ogg", _musicPath, _midiMapping[i].digitalTrack);
			debug(DBG_GAME, "playMusic('%s') track %s", name, filePath);
			File *f = new File;
			if (f->open(filePath)) {
				_mixer->playSoundVorbis(f, &_mixerMusicId);
			} else {
				delete f;
			}
			return;
		}
	}
	warning("Unable to find mapping for midi music '%s'", name);
}

void Game::changeObjectMotionFrame(int object, int object2, int useObject2, int count1, int count2, int useDx, int dx, int useDy, int dy) {
	debug(DBG_GAME, "Game::changeObjectMotionFrame()");
	SceneObject *so = derefSceneObject(object);
	if (so->statePrev != 0) {
		int num;
		if (useObject2) {
			num = derefSceneObject(object2)->motionInit;
		} else {
			num = _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstMotionIndex;
		}
		so->motionNum2 = num + count2 - 1;
		so->frameNum = _sceneObjectMotionsTable[so->motionNum2].firstFrameIndex + count1 - 1;
		if (so->flipPrev == 2) {
			int x = so->xPrev + _sceneObjectFramesTable[so->frameNumPrev].hdr.xPos;
			if (useDx) {
				x -= dx;
			} else {
				x -= _sceneObjectFramesTable[so->frameNum].hdr.xPos;
			}
			so->x = x + _sceneObjectFramesTable[so->frameNumPrev].hdr.w - _sceneObjectFramesTable[so->frameNum].hdr.w;
/*			int y = so->yPrev - _sceneObjectFramesTable[so->frameNumPrev].hdr.yPos;
			if (useDy) {
				y += dy;
			} else {
				y += _sceneObjectFramesTable[so->frameNum].hdr.yPos;
			}
			so->y = y;*/
		} else {
			int x = so->xPrev - _sceneObjectFramesTable[so->frameNumPrev].hdr.xPos;
			if (useDx) {
				x += dx;
			} else {
				x += _sceneObjectFramesTable[so->frameNum].hdr.xPos;
			}
			so->x = x;
/*			int y = so->yPrev - _sceneObjectFramesTable[so->frameNumPrev].hdr.yPos;
			if (useDy) {
				y += dy;
			} else {
				y += _sceneObjectFramesTable[so->frameNum].hdr.yPos;
			}
			so->y = y;*/
		}
		int y = so->yPrev - _sceneObjectFramesTable[so->frameNumPrev].hdr.yPos;
		if (useDy) {
			y += dy;
		} else {
			y += _sceneObjectFramesTable[so->frameNum].hdr.yPos;
		}
		so->y = y;
	}
}

int16 Game::getObjectTransformXPos(int object) {
	debug(DBG_GAME, "Game::getObjectTransformXPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	const int w = derefSceneObjectFrame(so->frameNumPrev)->hdr.w;

	int16 a0 = _objectScript.fetchNextWord();
	int16 a2 = _objectScript.fetchNextWord();
	int16 a4 = _objectScript.fetchNextWord();

	int16 dx = a0 * w / a2 + a4;
	if (so->flipPrev == 2) {
		dx = w - dx - 1;
	}
	return so->xPrev + dx;
}

int16 Game::getObjectTransformYPos(int object) {
	debug(DBG_GAME, "Game::getObjectTransformYPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	const int h = derefSceneObjectFrame(so->frameNumPrev)->hdr.h;

	int16 a0 = _objectScript.fetchNextWord();
	int16 a2 = _objectScript.fetchNextWord();
	int16 a4 = _objectScript.fetchNextWord();

	int16 dy = a0 * h / a2 + a4;
	if (so->flipPrev == 1) {
		dy = h - dy - 1;
	}
	return so->yPrev + dy;
}

bool Game::comparePrevObjectTransformXPos(int object, bool fetchCmp, int cmpX) {
	debug(DBG_GAME, "Game::comparePrevObjectTransformXPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	const int w = derefSceneObjectFrame(so->frameNumPrev)->hdr.w;

	int16 a0 = _objectScript.fetchNextWord();
	int16 a2 = _objectScript.fetchNextWord();
	int16 a4 = _objectScript.fetchNextWord();
	int16 a6 = _objectScript.fetchNextWord();
	int16 a8 = _objectScript.fetchNextWord();
	int16 aA = _objectScript.fetchNextWord();
	if (fetchCmp) {
		cmpX = _objectScript.fetchNextWord();
	}

	int16 xmin = a0 * w / a2 + a4;
	int16 xmax = a6 * w / a8 + aA;
	if (so->flipPrev == 2) {
		xmin = w - xmin;
		xmax = w - xmax;
	}
	if (xmax < xmin) {
		SWAP(xmax, xmin);
	}
	return so->statePrev != 0 && so->xPrev + xmin <= cmpX && so->xPrev + xmax >= cmpX;
}

bool Game::compareObjectTransformXPos(int object, bool fetchCmp, int cmpX) {
	debug(DBG_GAME, "Game::compareObjectTransformXPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	const int w = derefSceneObjectFrame(so->frameNum)->hdr.w;

	int16 a0 = _objectScript.fetchNextWord();
	int16 a2 = _objectScript.fetchNextWord();
	int16 a4 = _objectScript.fetchNextWord();
	int16 a6 = _objectScript.fetchNextWord();
	int16 a8 = _objectScript.fetchNextWord();
	int16 aA = _objectScript.fetchNextWord();
	if (fetchCmp) {
		cmpX = _objectScript.fetchNextWord();
	}

	int16 xmin = a0 * w / a2 + a4;
	int16 xmax = a6 * w / a8 + aA;
	if (so->flip == 2) {
		xmin = w - xmin;
		xmax = w - xmax;
	}
	if (xmax < xmin) {
		SWAP(xmax, xmin);
	}
	return so->state != 0 && so->x + xmin <= cmpX && so->x + xmax >= cmpX;
}

bool Game::comparePrevObjectTransformYPos(int object, bool fetchCmp, int cmpY) {
	debug(DBG_GAME, "Game::comparePrevObjectTransformYPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	const int h = derefSceneObjectFrame(so->frameNumPrev)->hdr.h;

	int16 a0 = _objectScript.fetchNextWord();
	int16 a2 = _objectScript.fetchNextWord();
	int16 a4 = _objectScript.fetchNextWord();
	int16 a6 = _objectScript.fetchNextWord();
	int16 a8 = _objectScript.fetchNextWord();
	int16 aA = _objectScript.fetchNextWord();
	if (fetchCmp) {
		cmpY = _objectScript.fetchNextWord();
	}

	int16 ymin = a0 * h / a2 + a4;
	int16 ymax = a6 * h / a8 + aA;
	if (so->flipPrev == 1) {
		ymin = h - ymin;
		ymax = h - ymax;
	}
	if (ymax < ymin) {
		SWAP(ymax, ymin);
	}
	return so->statePrev != 0 && so->yPrev + ymin <= cmpY && so->yPrev + ymax >= cmpY;
}

bool Game::compareObjectTransformYPos(int object, bool fetchCmp, int cmpY) {
	debug(DBG_GAME, "Game::compareObjectTransformYPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	const int h = derefSceneObjectFrame(so->frameNum)->hdr.h;

	int16 a0 = _objectScript.fetchNextWord();
	int16 a2 = _objectScript.fetchNextWord();
	int16 a4 = _objectScript.fetchNextWord();
	int16 a6 = _objectScript.fetchNextWord();
	int16 a8 = _objectScript.fetchNextWord();
	int16 aA = _objectScript.fetchNextWord();
	if (fetchCmp) {
		cmpY = _objectScript.fetchNextWord();
	}

	int16 ymin = a0 * h / a2 + a4;
	int16 ymax = a6 * h / a8 + aA;
	if (so->flip == 1) {
		ymin = h - ymin;
		ymax = h - ymax;
	}
	if (ymax < ymin) {
		SWAP(ymax, ymin);
	}
	return so->state != 0 && so->y + ymin <= cmpY && so->y + ymax >= cmpY;
}

void Game::setupObjectPos(int object, int object2, int useObject2, int useData, int type1, int type2) {
	debug(DBG_GAME, "Game::setupObjectPos(%d)", object);
	SceneObject *so = derefSceneObject(object);
	SceneObjectFrame *sof = derefSceneObjectFrame(so->frameNumPrev);
	int16 a0, a2, a4;
	int16 dy = 0, dx = 0, xmin = 0, xmax, ymin = 0, ymax, _ax;
	if (so->statePrev != 0) {
		if (type1 == 2) {
			a0 = _objectScript.fetchNextWord();
			a2 = _objectScript.fetchNextWord();
			a4 = _objectScript.fetchNextWord();
			xmin = a0 * sof->hdr.w / a2 + a4;
			a0 = _objectScript.fetchNextWord();
			a2 = _objectScript.fetchNextWord();
			a4 = _objectScript.fetchNextWord();
			xmax = a0 * sof->hdr.w / a2 + a4;
			if (xmax < xmin) {
				SWAP(xmin, xmax);
			}
			dx = xmax - xmin;
		}
		if (type2 == 2) {
			a0 = _objectScript.fetchNextWord();
			a2 = _objectScript.fetchNextWord();
			a4 = _objectScript.fetchNextWord();
			ymin = a0 * sof->hdr.h / a2 + a4;
			a0 = _objectScript.fetchNextWord();
			a2 = _objectScript.fetchNextWord();
			a4 = _objectScript.fetchNextWord();
			ymax = a0 * sof->hdr.h / a2 + a4;
			if (ymax < ymin) {
				SWAP(ymin, ymax);
			}
			dy = ymax - ymin;
		}

		if (useObject2 == 0) {
			_ax = _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstMotionIndex;
		} else {
			_ax = _sceneObjectsTable[object2].motionInit;
		}
		so->motionNum2 = _ax + _objectScript.fetchNextWord() - 1;
		if (useData == 0) {
			so->frameNum = _sceneObjectMotionsTable[so->motionNum2].firstFrameIndex;
		} else {
			so->frameNum = _sceneObjectMotionsTable[so->motionNum2].firstFrameIndex + _objectScript.fetchNextWord() - 1;
		}

		int16 _si = _sceneObjectFramesTable[so->frameNum].hdr.xPos - sof->hdr.xPos;
		if (type1 == 2) {
			 _si = ((xmin - dx + 1) / dx) * dx + _si % dx;
			if (_si < xmin) {
				_si += dx;
			}
		} else if (type1 == 3) {
			a0 = _objectScript.fetchNextWord();
			_objectScript.fetchNextWord();
			_si = a0 - sof->hdr.xPos;
		}

		int16 _di = _sceneObjectFramesTable[so->frameNum].hdr.yPos - sof->hdr.yPos;
		if (type2 == 2) {
			_di = ((ymin - dy + 1) / dy) * dy + _di % dy;
			if (_di < ymin) {
				_di += dy;
			}
		} else if (type2 == 3) {
			_objectScript.fetchNextWord();
			a2 = _objectScript.fetchNextWord();
			_di = a2 - sof->hdr.yPos;
		}

		if (so->flipPrev == 2) {
			_ax = so->xPrev - _si;
			_ax += _sceneObjectFramesTable[so->frameNumPrev].hdr.w;
			_ax -= _sceneObjectFramesTable[so->frameNum].hdr.w;
		} else {
			_ax = so->xPrev + _si;
		}
		so->x = _ax;
		so->y = so->yPrev + _di;
	}
}

bool Game::intersectsBox(int num, int index, int x1, int y1, int x2, int y2) {
	const Box *b = derefBox(num, index);
	if (b->state == 1) {
		if (b->x1 <= x1 && b->x2 >= x1 && b->y1 <= y1 && b->y2 >= y1) {
			return true;
		}
		if (b->x1 <= x2 && b->x2 >= x2 && b->y1 <= y2 && b->y2 >= y2) {
			return true;
		}
		if (b->x2 >= MIN(x1, x2) && b->x1 <= MAX(x1, x2) && b->y2 >= MIN(y1, y2) && b->y1 <= MAX(y1, y2)) {
			if (x1 == x2 || y1 == y2) {
				return true;
			}
			int iy = y1 - (y1 - y2) * (x1 - b->x1) / (x1 - x2);
			if (b->y1 <= iy && b->y2 >= iy && MIN(y1, y2) <= iy && MAX(y1, y2) >= iy) {
				return true;
			}
			iy = y1 - (y1 - y2) * (x1 - b->x2) / (x1 - x2);
			if (b->y1 <= iy && b->y2 >= iy && MIN(y1, y2) <= iy && MAX(y1, y2) >= iy) {
				return true;
			}
			int ix = x1 - (x1 - x2) * (y1 - b->y1) / (y1 - y2);
			if (b->x1 <= ix && b->x2 >= ix && MIN(x1, x2) <= ix && MAX(x1, x2) >= ix) {
				return true;
			}
			ix = x1 - (x1 - x2) * (y1 - b->y2) / (y1 - y2);
			if (b->x1 <= ix && b->x2 >= ix && MIN(x1, x2) <= ix && MAX(x1, x2) >= ix) {
				return true;
			}
		}
	}
	return false;
}
