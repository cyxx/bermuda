/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifndef GAME_H__
#define GAME_H__

#include "intern.h"
#include "random.h"
#include "fs.h"

struct SceneBitmap {
	uint16_t w; // x2
	uint16_t h; // y2
	uint16_t pitch;
	uint8_t *bits;
};

struct SceneAnimation {
	char name[20];
	int16_t firstMotionIndex;
	int16_t motionsCount;
	int16_t firstObjectIndex;
	int16_t objectsCount;
	int16_t firstSoundBufferIndex;
	int16_t soundBuffersCount;
	uint8_t *scriptData;
	uint16_t scriptSize;
	int16_t unk26;
};

struct SceneObject {
	int16_t xInit;
	int16_t yInit;
	int16_t x;
	int16_t y;
	int16_t xPrev;
	int16_t yPrev;
	int16_t zInit;
	int16_t zPrev;
	int16_t z;
	int16_t frameNum;
	int16_t frameNumPrev;
	int16_t flipInit;
	int16_t flip;
	int16_t flipPrev;
	int16_t motionNum; // motionStart
	int16_t motionInit;
	int16_t motionNum1; // motionNumPrev
	int16_t motionNum2; // motionNum
	int16_t motionFrameNum; // motionIdle ?
	int16_t mode;
	int16_t modeRndMul;
	int16_t statePrev;
	int16_t state;
	char name[20];
	char className[20];
	int16_t varsTable[10];
};

struct SoundBuffer {
	char filename[40];
//	uint8_t *buffer; // original preload the sound
};

struct SceneObjectMotion {
	int16_t firstFrameIndex;
	int16_t count;
	int16_t animNum;
};

struct Box {
	int16_t x1;
	int16_t x2;
	int16_t y1;
	int16_t y2;
	uint8_t state;
	int16_t z;
	int16_t startColor;
	int16_t endColor;
};

struct SceneObjectFrameHeader {
	int16_t num; // 0
	int16_t w; // 2
	int16_t h; // 4
	int16_t xPos; // 6
	int16_t yPos; // 8
};

struct SceneObjectFrame {
	uint8_t *data;
	SceneObjectFrameHeader hdr;
	int (*decode)(const uint8_t *, uint8_t *);
};

struct BagObject {
	char name[20];
	uint8_t *data;
	uint32_t dataSize; // not in the original
};

struct NextScene {
	int16_t num;
	char name[20];
};

struct SceneObjectStatus {
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t motionNum;
	int16_t frameNum;
	int16_t flip;
};

struct Script {
	uint8_t *data;
	int dataOffset;
	int testDataOffset;
	int currentObjectNum;
	bool objectFound;
	int testObjectNum;
	int nextScene;
	int statementNum;

	int16_t fetchNextWord() {
		int16_t word = READ_LE_UINT16(data + dataOffset);
		dataOffset += 2;
		return word;
	}

	const char *fetchNextString() {
		int len = fetchNextWord();
		assert(len >= 1);
		const char *str = (const char *)(data + dataOffset);
		dataOffset += len;
		assert(len >= 1 && str[len - 1] == '\0');
		return str;
	}

	const char *getString() {
		return (const char *)(data + dataOffset);
	}
};

struct DialogueChoice {
	char *id;
	bool gotoFlag;
	char *nextId;
	char *speechSoundFile;
	char *text;
};

struct Game;

enum {
	kActionTake = 0,
	kActionTalk = 1,
	kActionGirl = 2,
	kActionUseObject = 3,
};

enum {
	kObjectFlipY = 1,
	kObjectFlipX = 2
};

enum {
	kVarHasSword = 1,
	kVarHasGun = 2,
	kVarAmmo = 3
};

enum {
	kLeftMouseButton = 1 << 0,
	kRightMouseButton = 1 << 1
};

enum {
	kStateInit,
	kStateGame,
	kStateBag,
	kStateDialogue,
	kStateBitmap,
	kStateMenu1,
	kStateMenu2,
};

enum {
	// menu1
	kMenuOptionNewGame = 0,
	kMenuOptionLoadGame,
	kMenuOptionSaveGame,
	kMenuOptionQuitGame,
	// menu2
	kMenuOptionExitGame = 0,
	kMenuOptionReturnGame,
};

enum {
	kCycleDelay = 50,
	kGameScreenWidth = 640,
	kGameScreenHeight = 480,
	kDemoSavSlot = -1,
	kOffsetBitmapInfo = 0,
	kOffsetBitmapPalette = kOffsetBitmapInfo + 40,
	kOffsetBitmapBits = kOffsetBitmapPalette + 256 * 4,
	kBitmapBufferDefaultSize = 40 + 256 * 4 + 640 * 480 + 256 * 4
};

enum {
	kCheatNoHit = 1 << 0
};

static inline int getBitmapWidth(const uint8_t *p) {
	return READ_LE_UINT16(p) + 1;
}

static inline int getBitmapHeight(const uint8_t *p) {
	return READ_LE_UINT16(p + 2) + 1;
}

static inline int getBitmapSize(const uint8_t *p) {
	int sz = 4 + getBitmapWidth(p) * getBitmapHeight(p);
	return sz;
}

static inline const uint8_t *getBitmapData(const uint8_t *p) {
	return p + 4;
}

static inline bool boxInRect(Box *box, int rect_x1, int rect_x2, int rect_y1, int rect_y2) {
	assert(box->x1 <= box->x2 && box->y1 <= box->y2);
	const int xmin = MIN(rect_x1, rect_x2);
	const int xmax = MAX(rect_x1, rect_x2);
	const int ymin = MIN(rect_y1, rect_y2);
	const int ymax = MAX(rect_y1, rect_y2);
	return box->x1 <= xmax && box->x2 >= xmin && box->y1 <= ymax && box->y2 >= ymin;
}

struct Mixer;
struct SystemStub;

struct Game {
	enum {
		NUM_VARS = 310,
		NUM_BOXES = 20,
		NUM_SCENE_OBJECT_FRAMES = 3000,
		NUM_SCENE_MOTIONS = 300,
		NUM_SOUND_BUFFERS = 80,
		NUM_SCENE_OBJECTS = 50,
		NUM_SCENE_ANIMATIONS = 50,
		NUM_BAG_OBJECTS = 20,
		NUM_NEXT_SCENES = 20,
		NUM_SCENE_OBJECT_STATUS = 200,
		NUM_DIALOG_CHOICES = 10,
		NUM_DIALOG_ENTRIES = 40
	};

	Game(SystemStub *stub, const char *dataPath, const char *savePath, const char *musicPath);
	~Game();

	SceneObject *derefSceneObject(int i) {
		assert(i >= 0 && i < _sceneObjectsCount);
		return &_sceneObjectsTable[i];
	}

	Box *derefBox(int i, int j) {
		assert(i >= 0 && i < NUM_BOXES && j >= 0 && j < 10);
		return &_boxesTable[i][j];
	}

	SceneObjectStatus *derefSceneObjectStatus(int i) {
		assert(i >= 0 && i < NUM_SCENE_OBJECT_STATUS);
		return &_sceneObjectStatusTable[i];
	}

	SceneObjectFrame *derefSceneObjectFrame(int i) {
		assert(i >= 0 && i < _sceneObjectFramesCount);
		return &_sceneObjectFramesTable[i];
	}

	// bag.cpp
	void handleBagMenu();
	void drawBagMenu(int xPosWnd, int yPosWnd);

	// dialogue.cpp
	void unloadDialogueData();
	void setupDialog(const char *code);
	void loadDialogueSprite(int spr);
	void loadDialogueData(const char *filename);
	void redrawDialogueSprite(int num);
	void redrawDialogueBackground();
	void redrawDialogueTexts();
	void initDialogue();
	void finiDialogue();
	void handleDialogue();

	// game.cpp
	void detectTextCp949();
	void detectVersion();
	void restart();
	void init(bool fullscreen, int screenMode);
	void fini();
	void mainLoop();
	void updateMouseButtonsPressed();
	void updateKeysPressedTable();
	void clearSceneData(int anim);
	void reinitializeObject(int object);
	void updateObjects();
	void runObjectsScript();
	int findBagObjectByName(const char *objectName) const;
	int getObjectTranslateXPos(int object, int dx1, int div, int dx2);
	int getObjectTranslateYPos(int object, int dy1, int div, int dy2);
	int findObjectByName(int currentObjectNum, int defaultObjectNum, bool *objectFlag);
	void sortObjects();
	void copyBufferToBuffer(int x, int y, int w, int h, SceneBitmap *src, SceneBitmap *dst);
	void drawBox(int x, int y, int w, int h, SceneBitmap *src, SceneBitmap *dst, int startColor, int endColor);
	void drawObject(int x, int y, const uint8_t *src, SceneBitmap *dst);
	void drawObjectVerticalFlip(int x, int y, const uint8_t *src, SceneBitmap *dst);
	void redrawObjectBoxes(int previousObject, int currentObject);
	void redrawObjects();
	void playVideo(const char *name);
	void displayTitleBitmap();
	void stopMusic();
	void playMusic(const char *name);
	void changeObjectMotionFrame(int object, int object2, int useObject2, int count1, int count2, int useDx, int dx, int useDy, int dy);
	int16_t getObjectTransformXPos(int object);
	int16_t getObjectTransformYPos(int object);
	bool comparePrevObjectTransformXPos(int object, bool fetchCmp = true, int cmpX = -1);
	bool compareObjectTransformXPos(int object, bool fetchCmp = true, int cmpX = -1);
	bool comparePrevObjectTransformYPos(int object, bool fetchCmp = true, int cmpY = -1);
	bool compareObjectTransformYPos(int object, bool fetchCmp = true, int cmpY = -1);
	void setupObjectPos(int object, int object2, int useObject2, int useData, int type1, int type2);
	bool intersectsBox(int num, int index, int x1, int y1, int x2, int y2);

	// menu.cpp
	void initMenu(int num);
	void finiMenu();
	void handleMenu();

	// opcodes.cpp
	bool executeConditionOpcode(int num);
	void executeOperatorOpcode(int num);
	void evalExpr(int16_t *val);
	bool testExpr(int16_t val);
	bool cop_true();
	bool cop_isInRandomRange();
	bool cop_isKeyPressed();
	bool cop_isKeyNotPressed();
	bool cop_testMouseButtons();
	bool cop_isObjectInScene();
	bool cop_testObjectPrevState();
	bool cop_testObjectState();
	bool cop_isObjectInRect();
	bool cop_testPrevObjectTransformXPos();
	bool cop_testObjectTransformXPos();
	bool cop_testPrevObjectTransformYPos();
	bool cop_testObjectTransformYPos();
	bool cop_testObjectPrevFlip();
	bool cop_testObjectFlip();
	bool cop_testObjectPrevFrameNum();
	bool cop_testObjectFrameNum();
	bool cop_testPrevMotionNum();
	bool cop_testMotionNum();
	bool cop_testObjectVar();
	bool cop_testObjectAndObjectXPos();
	bool cop_testObjectAndObjectYPos();
	bool cop_testObjectMotionYPos();
	bool cop_testVar();
	bool cop_isCurrentBagAction();
	bool cop_isObjectInBox();
	bool cop_isObjectNotInBox();
	bool cop_isObjectNotIntersectingBox();
	bool cop_isCurrentBagObject();
	bool cop_isLifeBarDisplayed();
	bool cop_isLifeBarNotDisplayed();
	bool cop_testLastDialogue();
	bool cop_isNextScene();
	void oop_initializeObject();
	void oop_evalCurrentObjectX();
	void oop_evalCurrentObjectY();
	void oop_evalObjectX();
	void oop_evalObjectY();
	void oop_evalObjectZ();
	void oop_setObjectFlip();
	void oop_adjustObjectPos_vv0000();
	void oop_adjustObjectPos_vv1v00();
	void oop_adjustObjectPos_vv1v1v();
	void oop_setupObjectPos_121();
	void oop_setupObjectPos_122();
	void oop_setupObjectPos_123();
	void oop_adjustObjectPos_1v0000();
	void oop_adjustObjectPos_1v1v1v();
	void oop_setupObjectPos_021();
	void oop_setupObjectPos_022();
	void oop_setupObjectPos_023();
	void oop_evalObjectVar();
	void oop_translateObjectXPos();
	void oop_translateObjectYPos();
	void oop_setObjectMode();
	void oop_setObjectInitPos();
	void oop_setObjectTransformInitPos();
	void oop_evalObjectXInit();
	void oop_evalObjectYInit();
	void oop_evalObjectZInit();
	void oop_setObjectFlipInit();
	void oop_setObjectCel();
	void oop_resetObjectCel();
	void oop_evalVar();
	void oop_getSceneNumberInVar();
	void oop_disableBox();
	void oop_enableBox();
	void oop_evalBoxesXPos();
	void oop_evalBoxesYPos();
	void oop_setBoxToObject();
	void oop_clipBoxes();
	void oop_saveObjectStatus();
	void oop_addObjectToBag();
	void oop_removeObjectFromBag();
	void oop_playSoundLowerEqualPriority();
	void oop_playSoundLowerPriority();
	void oop_startDialogue();
	void oop_switchSceneClearBoxes();
	void oop_switchSceneCopyBoxes();

	// parser_scn.cpp
	void parseSCN(const char *fileName);

	// parser_dlg.cpp
	void parseDLG();

	// resource.cpp
	void allocateTables();
	void deallocateTables();
	void loadCommonSprites();
	void unloadCommonSprites();
	uint8_t *loadFile(const char *fileName, uint8_t *dst = 0, uint32_t *dstSize = 0);
	void loadWGP(const char *fileName);
	void loadSPR(const char *fileName, SceneAnimation *sa);
	void loadMOV(const char *fileName);
	void loadKBR(const char *fileName);
	void loadTBM();

	// saveload.cpp
	void saveState(File *f, int slot);
	void loadState(File *f, int slot, bool switchScene);

	// win16.cpp (temporary helpers)
	int win16_sndPlaySound(int op, void *data = 0);
	void win16_stretchBits(SceneBitmap *bits, int srcHeight, int srcWidth, int srcY, int srcX, int dstHeight, int dstWidth, int dstY, int dstX);

	int _nextState, _state;
	bool _textCp949;
	bool _isDemo;
	const char *_startupScene;
	FileSystem _fs;
	RandomGenerator _rnd;
	SystemStub *_stub;
	Mixer *_mixer;
	const char *_dataPath;
	const char *_savePath;
	const char *_musicPath;
	uint32_t _cheats;
	int _stateSlot;
	int _mixerSoundId;
	int _mixerMusicId;
	int _menuObjectCount;
	int _menuObjectMotion;
	int _menuObjectFrames;
	int _menuOption;
	int _menuHighlight;

	uint8_t *_bitmapBuffer0;
	SceneBitmap _bitmapBuffer1;
	uint8_t *_bitmapBuffer2;
	SceneBitmap _bitmapBuffer3;
	uint8_t *_bermudaOvrData;
	uint8_t *_tempDecodeBuffer;
	SceneBitmap _bagBackgroundImage;
	uint8_t *_bermudaSprData;
	uint8_t *_bermudaSprDataTable[3];
	uint32_t _keyboardReplaySize;
	uint32_t _keyboardReplayOffset;
	uint8_t *_keyboardReplayData;

	uint8_t *_hangulFontData;
	uint32_t _hangulFontLutOffset;

	// inventory
	uint8_t *_lifeBarImageTable[11][12];
	uint8_t *_weaponIconImageTable[14];
	uint8_t *_ammoIconImageTable[2][5];
	uint8_t *_swordIconImage;
	uint8_t *_iconBackgroundImage;
	uint8_t *_lifeBarImage;
	uint8_t *_bagObjectAreaBlinkImageTable[10];
	int _bagObjectAreaBlinkCounter;
	uint8_t *_bagWeaponAreaBlinkImageTable[10];
	int _bagWeaponAreaBlinkCounter;
	int _lifeBarCurrentFrame;
	int _bagObjectW, _bagObjectH;

	// dialogue
	int _lastDialogueEndedId;
	int _dialogueEndedFlag;
	int _dialogueSpriteIndex;
	int _dialogueSpriteFrameCountTable[3];
	int _dialogueSpriteCurrentFrameTable[3];
	int _dialogueChoiceSelected;
	uint8_t *_dialogueSpriteDataTable[3][105];
	int _dialogueSpeechIndex;
	int _loadDialogueDataState;
	int _dialogueDescriptionSize;
	char *_dialogueDescriptionBuffer;
	DialogueChoice _dialogueChoiceData[NUM_DIALOG_ENTRIES];
	int _dialogueChoiceSize;
	bool _dialogueChoiceGotoFlag[NUM_DIALOG_CHOICES];
	char *_dialogueChoiceText[NUM_DIALOG_CHOICES];
	char *_dialogueChoiceSpeechSoundFile[NUM_DIALOG_CHOICES];
	char *_dialogueChoiceNextId[NUM_DIALOG_CHOICES];
	int _dialogueChoiceCounter;
	uint8_t *_dialogueFrameSpriteData;
	Rect _dialogueTextRect;

	// logic
	int _sceneDescriptionSize;
	char *_sceneDescriptionBuffer;
	Script _objectScript;
	int16_t _defaultVarsTable[NUM_VARS];
	int16_t _varsTable[NUM_VARS];
	const char *_scriptDialogId;
	const char *_scriptDialogFileName;
	const char *_scriptDialogSprite1;
	const char *_scriptDialogSprite2;
	bool _switchScene;
	bool _startDialogue;
	bool _loadState;
	bool _clearSceneData;
	bool _gameOver;
	int _loadDataState;
	int _currentBagAction;
	int _previousBagAction;
	int _currentBagObject;
	int _previousBagObject;
	bool _workaroundRaftFlySceneBug;
	int _currentPlayingSoundPriority;
	bool _lifeBarDisplayed;
	bool _lifeBarDisplayed2;
	uint8_t _keysPressed[128];
	int _mouseButtonsPressed;
	int _musicTrack;
	char _musicName[40];
	int _sceneNumber;
	char _currentSceneWgp[128];
	char _tempTextBuffer[128];
	char _currentSceneScn[128];
	char _currentSceneSav[128]; // non interactive parts of the demo relies on savestates
	int _bagPosX, _bagPosY;
	SceneObject *_sortedSceneObjectsTable[NUM_SCENE_OBJECTS];
	SceneObject _sceneObjectsTable[NUM_SCENE_OBJECTS];
	int _sceneObjectsCount;
	SceneAnimation _animationsTable[NUM_SCENE_ANIMATIONS];
	int _animationsCount;
	SoundBuffer _soundBuffersTable[NUM_SOUND_BUFFERS];
	int _soundBuffersCount;
	Box _boxesTable[NUM_BOXES][10];
	int _boxesCountTable[NUM_BOXES];
	SceneObjectFrame _sceneObjectFramesTable[NUM_SCENE_OBJECT_FRAMES];
	int _sceneObjectFramesCount;
	BagObject _bagObjectsTable[NUM_BAG_OBJECTS];
	int _bagObjectsCount;
	SceneObjectMotion _sceneObjectMotionsTable[NUM_SCENE_MOTIONS];
	int _sceneObjectMotionsCount;
	NextScene _nextScenesTable[NUM_NEXT_SCENES];
	int _sceneConditionsCount;
	SceneObjectStatus _sceneObjectStatusTable[NUM_SCENE_OBJECT_STATUS];
	int _sceneObjectStatusCount;

	static const uint16_t _fontData[];
	static const uint8_t _fontCharWidth[];
	static const uint8_t _bermudaIconBmpData[];
	static const int _bermudaIconBmpSize;
	static const uint8_t _bermudaDemoBmpData[];
	static const int _bermudaDemoBmpSize;
};

#endif // GAME_H__
