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
	uint16 w; // x2
	uint16 h; // y2
	uint16 pitch;
	uint8 *bits;
};

struct SceneAnimation {
	char name[20];
	int16 firstMotionIndex;
	int16 motionsCount;
	int16 firstObjectIndex;
	int16 objectsCount;
	int16 firstSoundBufferIndex;
	int16 soundBuffersCount;
	uint8 *scriptData;
	uint16 scriptSize;
	int16 unk26;
};

struct SceneObject {
	int16 xInit;
	int16 yInit;
	int16 x;
	int16 y;
	int16 xPrev;
	int16 yPrev;
	int16 zInit;
	int16 zPrev;
	int16 z;
	int16 frameNum;
	int16 frameNumPrev;
	int16 flipInit;
	int16 flip;
	int16 flipPrev;
	int16 motionNum; // motionStart
	int16 motionInit;
	int16 motionNum1; // motionNumPrev
	int16 motionNum2; // motionNum
	int16 motionFrameNum; // motionIdle ?
	int16 mode;
	int16 modeRndMul;
	int16 statePrev;
	int16 state;
	char name[20];
	char className[20];
	int16 varsTable[10];
};

struct SoundBuffer {
	char filename[40];
//	uint8 *buffer; // original preload the sound
};

struct SceneObjectMotion {
	int16 firstFrameIndex;
	int16 count;
	int16 animNum;
};

struct Box {
	int16 x1;
	int16 x2;
	int16 y1;
	int16 y2;
	uint8 state;
	int16 z;
	int16 startColor;
	int16 endColor;
};

struct SceneObjectFrameHeader {
	int16 num; // 0
	int16 w; // 2
	int16 h; // 4
	int16 xPos; // 6
	int16 yPos; // 8
};

struct SceneObjectFrame {
	uint8 *data;
	SceneObjectFrameHeader hdr;
};

struct BagObject {
	char name[20];
	uint8 *data;
	uint32 dataSize; // not in the original
};

struct NextScene {
	int16 num;
	char name[20];
};

struct SceneObjectStatus {
	int16 x;
	int16 y;
	int16 z;
	int16 motionNum;
	int16 frameNum;
	int16 flip;
};

struct Script {
	uint8 *data;
	int dataOffset;
	int testDataOffset;
	int currentObjectNum;
	bool objectFound;
	int testObjectNum;
	int nextScene;
	int statementNum;

	int16 fetchNextWord() {
		int16 word = READ_LE_UINT16(data + dataOffset);
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

struct GameConditionOpcode {
	int num;
	bool (Game::*pf)();
};

struct GameOperatorOpcode {
	int num;
	void (Game::*pf)();
};

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
	kCycleDelay = 50,
	kGameScreenWidth = 640,
	kGameScreenHeight = 480,
	kOffsetBitmapInfo = 0,
	kOffsetBitmapPalette = kOffsetBitmapInfo + 40,
	kOffsetBitmapBits = kOffsetBitmapPalette + 256 * 4,
	kBitmapBufferDefaultSize = 40 + 256 * 4 + 640 * 480 + 256 * 4
};

static inline int getBitmapWidth(const uint8 *p) {
	return READ_LE_UINT16(p) + 1;
}

static inline int getBitmapHeight(const uint8 *p) {
	return READ_LE_UINT16(p + 2) + 1;
}

static inline int getBitmapSize(const uint8 *p) {
	int sz = 4 + getBitmapWidth(p) * getBitmapHeight(p);
	return sz;
}

static inline const uint8 *getBitmapData(const uint8 *p) {
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
	void handleDialogue();

	// game.cpp
	void detectVersion();
	void restart();
	void mainLoop();
	void updateMouseButtonsPressed();
	void updateKeysPressedTable();
	void setupScreenPalette(const uint8 *src);
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
	void drawObject(int x, int y, const uint8 *src, SceneBitmap *dst);
	void drawObjectVerticalFlip(int x, int y, const uint8 *src, SceneBitmap *dst);
	void redrawObjectBoxes(int previousObject, int currentObject);
	void redrawObjects();
	void playVideo(const char *name);
	void playBitmapSequenceDemo();
	void stopMusic();
	void playMusic(const char *name);
	void changeObjectMotionFrame(int object, int object2, int useObject2, int count1, int count2, int useDx, int dx, int useDy, int dy);
	int16 getObjectTransformXPos(int object);
	int16 getObjectTransformYPos(int object);
	bool comparePrevObjectTransformXPos(int object, bool fetchCmp = true, int cmpX = -1);
	bool compareObjectTransformXPos(int object, bool fetchCmp = true, int cmpX = -1);
	bool comparePrevObjectTransformYPos(int object, bool fetchCmp = true, int cmpY = -1);
	bool compareObjectTransformYPos(int object, bool fetchCmp = true, int cmpY = -1);
	void setupObjectPos(int object, int object2, int useObject2, int useData, int type1, int type2);
	bool intersectsBox(int num, int index, int x1, int y1, int x2, int y2);

	// opcodes.cpp
	const GameConditionOpcode *findConditionOpcode(int num) const;
	const GameOperatorOpcode *findOperatorOpcode(int num) const;
	void evalExpr(int16 *val);
	bool testExpr(int16 val);
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
	uint8 *loadFile(const char *fileName, uint8 *dst = 0, uint32 *dstSize = 0);
	void loadWGP(const char *fileName);
	void loadSPR(const char *fileName, SceneAnimation *sa);
	void loadMOV(const char *fileName);

	// saveload.cpp
	void saveState(int slot);
	void loadState(int slot, bool switchScene);

	// win16.cpp (temporary helpers)
	int win16_sndPlaySound(int op, void *data = 0);
	void win16_stretchBits(SceneBitmap *bits, int srcHeight, int srcWidth, int srcY, int srcX, int dstHeight, int dstWidth, int dstY, int dstX);

	bool _isDemo;
	const char *_startupScene;
	FileSystem _fs;
	RandomGenerator _rnd;
	SystemStub *_stub;
	Mixer *_mixer;
	const char *_dataPath;
	const char *_savePath;
	const char *_musicPath;
	int _stateSlot;
	int _mixerSoundId;
	int _mixerMusicId;
	uint32 _lastFrameTimeStamp;

	uint8 *_bitmapBuffer0;
	SceneBitmap _bitmapBuffer1;
	uint8 *_bitmapBuffer2;
	SceneBitmap _bitmapBuffer3;
	uint8 *_bermudaOvrData;
	uint8 *_tempDecodeBuffer;
	SceneBitmap _bagBackgroundImage;
	uint8 *_bermudaSprData;
	uint8 *_bermudaSprDataTable[3];

	// inventory
	uint8 *_lifeBarImageTable[11][12];
	uint8 *_weaponIconImageTable[14];
	uint8 *_ammoIconImageTable[2][5];
	uint8 *_swordIconImage;
	uint8 *_iconBackgroundImage;
	uint8 *_lifeBarImage;
	uint8 *_bagObjectAreaBlinkImageTable[10];
	int _bagObjectAreaBlinkCounter;
	uint8 *_bagWeaponAreaBlinkImageTable[10];
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
	uint8 *_dialogueSpriteDataTable[3][105];
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
	uint8 *_dialogueFrameSpriteData;
	Rect _dialogueTextRect;

	// logic
	int _sceneDescriptionSize;
	char *_sceneDescriptionBuffer;
	Script _objectScript;
	int16 _defaultVarsTable[NUM_VARS];
	int16 _varsTable[NUM_VARS];
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
	uint8 _keysPressed[128];
	int _mouseButtonsPressed;
	int _musicTrack; // useless, always equal to 0
	char _musicName[40];
	int _sceneNumber;
	char _currentSceneWgp[128];
	char _tempTextBuffer[128];
	char _currentSceneScn[128];
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

	static const GameConditionOpcode _conditionOpTable[];
	static const int _conditionOpCount;
	static const GameOperatorOpcode _operatorOpTable[];
	static const int _operatorOpCount;
	static const uint16 _fontData[];
	static const uint8 _fontCharWidth[];
};

#endif // GAME_H__
