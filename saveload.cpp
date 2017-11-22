/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "file.h"
#include "game.h"

enum SaveLoadMode {
	kSaveMode,
	kLoadMode
};

static File *_saveOrLoadStream;
static SaveLoadMode _saveOrLoadMode;

static void saveByte(uint8_t b) {
	_saveOrLoadStream->writeByte(b);
}

static uint8_t loadByte() {
	return _saveOrLoadStream->readByte();
}

static void saveInt16(int i) {
	_saveOrLoadStream->writeUint16LE(i);
}

static int loadInt16() {
	return _saveOrLoadStream->readUint16LE();
}

static void saveInt32(int i) {
	_saveOrLoadStream->writeUint32LE(i);
}

static int loadInt32() {
	return _saveOrLoadStream->readUint32LE();
}

static void saveStr(void *s, int len) {
	_saveOrLoadStream->write(s, len);
}

static void loadStr(void *s, int len) {
	_saveOrLoadStream->read(s, len);
}

static void saveOrLoadByte(uint8_t &b) {
	switch (_saveOrLoadMode) {
	case kSaveMode:
		saveByte(b);
		break;
	case kLoadMode:
		b = loadByte();
		break;
	}
}

static void saveOrLoadInt16(int16_t &i) {
	switch (_saveOrLoadMode) {
	case kSaveMode:
		saveInt16(i);
		break;
	case kLoadMode:
		i = loadInt16();
		break;
	}
}

static void saveOrLoadStr(char *s, int16_t len = -1) {
	bool countTermChar = (len == -2); // deal with original buggy file format...
	bool storeLen = (len < 0);
	if (storeLen) {
		len = strlen(s);
		if (countTermChar) {
			++len;
		}
		saveOrLoadInt16(len);
	}
	switch (_saveOrLoadMode) {
	case kSaveMode:
		saveStr(s, len);
		break;
	case kLoadMode:
		loadStr(s, len);
		if (storeLen) {
			s[len] = 0;
		}
		break;
	}
}

static void saveOrLoad_sceneObject(SceneObject &so) {
	saveOrLoadInt16(so.xInit);
	saveOrLoadInt16(so.yInit);
	saveOrLoadInt16(so.x);
	saveOrLoadInt16(so.y);
	saveOrLoadInt16(so.xPrev);
	saveOrLoadInt16(so.yPrev);
	saveOrLoadInt16(so.zInit);
	saveOrLoadInt16(so.zPrev);
	saveOrLoadInt16(so.z);
	saveOrLoadInt16(so.frameNum);
	saveOrLoadInt16(so.frameNumPrev);
	saveOrLoadInt16(so.flipInit);
	saveOrLoadInt16(so.flip);
	saveOrLoadInt16(so.flipPrev);
	saveOrLoadInt16(so.motionNum);
	saveOrLoadInt16(so.motionInit);
	saveOrLoadInt16(so.motionNum1);
	saveOrLoadInt16(so.motionNum2);
	saveOrLoadInt16(so.motionFrameNum);
	saveOrLoadInt16(so.mode);
	saveOrLoadInt16(so.modeRndMul);
	saveOrLoadInt16(so.statePrev);
	saveOrLoadInt16(so.state);
	saveOrLoadStr(so.name, 20);
	saveOrLoadStr(so.className, 20);
	for (int j = 0; j < 10; ++j) {
		saveOrLoadInt16(so.varsTable[j]);
	}
}

static void saveOrLoad_box(Box &b) {
	saveOrLoadInt16(b.x1);
	saveOrLoadInt16(b.x2);
	saveOrLoadInt16(b.y1);
	saveOrLoadInt16(b.y2);
	saveOrLoadByte(b.state);
	saveOrLoadInt16(b.z);
	saveOrLoadInt16(b.startColor);
	saveOrLoadInt16(b.endColor);
}

static void saveOrLoad_sceneObjectStatus(SceneObjectStatus &sos) {
	saveOrLoadInt16(sos.x);
	saveOrLoadInt16(sos.y);
	saveOrLoadInt16(sos.z);
	saveOrLoadInt16(sos.motionNum);
	saveOrLoadInt16(sos.frameNum);
	saveOrLoadInt16(sos.flip);
}

// original save/load data format
static void save_bagObjects(BagObject *bo, int count) {
	int totalSize = 0;
	for (int i = 0; i < count; ++i) {
		totalSize += bo[i].dataSize;
	}
	assert(totalSize < 0xFFFF);
	saveInt16(totalSize);
	for (int i = 0; i < count; ++i) {
		saveStr(bo[i].data, bo[i].dataSize);
	}
	saveInt16(count);
	int offset = 0;
	for (int i = 0; i < count; ++i) {
		saveInt16(offset);
		saveStr(bo[i].name, 20);
		offset += bo[i].dataSize;
	}
}

static void load_bagObjects(BagObject *bo, int &count) {
	uint16_t totalSize = loadInt16();
	uint8_t *bagData = (uint8_t *)malloc(totalSize);
	if (bagData) {
		loadStr(bagData, totalSize);
		count = loadInt16();
		assert(count <= Game::NUM_BAG_OBJECTS);
		uint16_t bagObjectsOffset[Game::NUM_BAG_OBJECTS + 1];
		for (int i = 0; i < count; ++i) {
			bagObjectsOffset[i] = loadInt16();
			loadStr(bo[i].name, 20);
		}
		for (int i = 0; i < count; ++i) {
			int dataSize = (i == count - 1) ? totalSize : bagObjectsOffset[i + 1];
			dataSize -= bagObjectsOffset[i];
			bo[i].data = (uint8_t *)malloc(dataSize);
			if (bo[i].data) {
				memcpy(bo[i].data, bagData + bagObjectsOffset[i], dataSize);
				bo[i].dataSize = dataSize;
			}
		}
		free(bagData);
	}
}

void Game::saveState(File *f, int slot) {
	_saveOrLoadStream = f;
	_saveOrLoadMode = kSaveMode;

	saveInt16(NUM_VARS);
	for (int i = 0; i < NUM_VARS; ++i) {
		saveInt16(_defaultVarsTable[i]);
	}
	assert(strchr(_currentSceneScn, '\\') == 0);
	saveOrLoadStr(_currentSceneScn, -2);
	saveInt16(_sceneObjectsCount);
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		saveOrLoad_sceneObject(_sceneObjectsTable[i]);
	}
	saveInt16(NUM_BOXES);
	for (int i = 0; i < NUM_BOXES; ++i) {
		saveInt16(_boxesCountTable[i]);
	}
	saveInt16(NUM_BOXES * 10);
	for (int i = 0; i < NUM_BOXES; ++i) {
		for (int j = 0; j < 10; ++j) {
			saveOrLoad_box(_boxesTable[i][j]);
		}
	}
	saveInt16(NUM_VARS);
	for (int i = 0; i < NUM_VARS; ++i) {
		saveInt16(_varsTable[i]);
	}
	saveInt16(NUM_SCENE_OBJECT_STATUS);
	for (int i = 0; i < NUM_SCENE_OBJECT_STATUS; ++i) {
		saveOrLoad_sceneObjectStatus(_sceneObjectStatusTable[i]);
	}
	saveInt16(_bagPosX);
	saveInt16(_bagPosY);
	saveInt16(_currentBagObject);
	save_bagObjects(_bagObjectsTable, _bagObjectsCount);
	saveInt16(_currentBagAction);
	saveInt32(_musicTrack);
	saveInt32(_musicTrack); // original saved it twice...
	saveOrLoadStr(_musicName);
	debug(DBG_INFO, "Saved state to slot %d", slot);
}

void Game::loadState(File *f, int slot, bool switchScene) {
	_saveOrLoadStream = f;
	_saveOrLoadMode = kLoadMode;

	stopMusic();

	int n = loadInt16();
	assert(n <= NUM_VARS);
	for (int i = 0; i < n; ++i) {
		_varsTable[i] = loadInt16();
	}
	saveOrLoadStr(_tempTextBuffer, -2);
	if (switchScene) {
		_switchScene = true;
		return;
	}
	_sceneObjectsCount = loadInt16();
	for (int i = 0; i < _sceneObjectsCount; ++i) {
		saveOrLoad_sceneObject(_sceneObjectsTable[i]);
	}
	n = loadInt16();
	assert(n <= NUM_BOXES);
	for (int i = 0; i < n; ++i) {
		_boxesCountTable[i] = loadInt16();
	}
	n = loadInt16();
	assert((n % 10) == 0 && n <= NUM_BOXES * 10);
	for (int i = 0; i < n / 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			saveOrLoad_box(_boxesTable[i][j]);
		}
	}
	n = loadInt16();
	assert(n <= NUM_VARS);
	for (int i = 0; i < n; ++i) {
		_varsTable[i] = loadInt16();
	}
	n = loadInt16();
	memset(_sceneObjectStatusTable, 0, sizeof(_sceneObjectStatusTable));
	assert(n <= NUM_SCENE_OBJECT_STATUS);
	for (int i = 0; i < n; ++i) {
		saveOrLoad_sceneObjectStatus(_sceneObjectStatusTable[i]);
	}
	_bagPosX = loadInt16();
	_bagPosY = loadInt16();
	_currentBagObject = loadInt16();
	_previousBagObject = _currentBagObject;
	for (int i = 0; i < _bagObjectsCount; ++i) {
		free(_bagObjectsTable[i].data);
	}
	memset(_bagObjectsTable, 0, sizeof(_bagObjectsTable));
	load_bagObjects(_bagObjectsTable, _bagObjectsCount);
	_currentBagAction = loadInt16();
	loadInt32();
	// demo .SAV files do not persist any music state
	if (slot == kDemoSavSlot) {
		debug(DBG_INFO, "Loaded state from .SAV scene '%s'", _tempTextBuffer);
		return;
	}
	_musicTrack = loadInt32();
	saveOrLoadStr(_musicName);
	debug(DBG_INFO, "Loaded state from slot %d scene '%s'", slot, _tempTextBuffer);
}
