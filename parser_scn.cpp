/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "file.h"
#include "fs.h"
#include "game.h"
#include "str.h"

enum ParserState {
	kParserStateDef = 0,
	kParserStateMovies = 1,
	kParserStateBag = 2,
	kParserStateScene = 3,
	kParserStateObject = 4,
	kParserStateNewObject = 5,
	kParserStateBox = 6
};

enum ParserToken {
	kParserTokenGlobalMemory,
	kParserTokenSceneNumber,
	kParserTokenEq,
	kParserTokenNotEq,
	kParserTokenLower,
	kParserTokenLowerEq,
	kParserTokenGreater,
	kParserTokenGreaterEq,
	kParserTokenThen,
	kParserTokenEnd,
	kParserTokenMovies,
	kParserTokenScene,
	kParserTokenScreen,
	kParserTokenMidi,
	kParserTokenNULL,
	kParserTokenObject,
	kParserTokenIfNewObject,
	kParserTokenBag,
	kParserTokenBox,
	kParserTokenBagEnd,
	kParserTokenMoviesEnd,
	kParserTokenSceneEnd,
	kParserTokenObjectEnd,
	kParserTokenBoxEnd,
	kParserTokenClass,
	kParserTokenMemory,
	kParserTokenCoord,
	kParserTokenAddCoordX,
	kParserTokenAddCoordY,
	kParserTokenCoordX,
	kParserTokenCoordY,
	kParserTokenDepth,
	kParserTokenMove,
	kParserTokenCel,
	kParserTokenMirror,
	kParserTokenNo,
	kParserTokenX,
	kParserTokenY,
	kParserTokenXY,
	kParserTokenInit,
	kParserTokenNoInit,
	kParserTokenSimple,
	kParserTokenRandom,
	kParserTokenPut,
	kParserTokenLoadStatus,
	kParserTokenDisable,
	kParserTokenEnable,
	kParserTokenMix,
	kParserTokenEOS,
	kParserTokenUnknown
};

static int _currentState;
static bool _stopParsing;
static ParserToken _currentToken;
static const char *_currentTokenStr;
static SceneObject *_currentSceneObject;

static ParserToken getNextToken(char **s) {
	_currentTokenStr = stringNextToken(s);
	if (!_currentTokenStr || !*_currentTokenStr) {
		return kParserTokenEOS;
	} else if (strcmp(_currentTokenStr, "GlobalMemory") == 0) {
		return kParserTokenGlobalMemory;
	} else if (strcmp(_currentTokenStr, "SceneNumber") == 0) {
		return kParserTokenSceneNumber;
	} else if (strcmp(_currentTokenStr, "==") == 0) {
		return kParserTokenEq;
	} else if (strcmp(_currentTokenStr, "!=") == 0) {
		return kParserTokenNotEq;
	} else if (strcmp(_currentTokenStr, "<") == 0) {
		return kParserTokenLower;
	} else if (strcmp(_currentTokenStr, "<=") == 0) {
		return kParserTokenLowerEq;
	} else if (strcmp(_currentTokenStr, ">") == 0) {
		return kParserTokenGreater;
	} else if (strcmp(_currentTokenStr, ">=") == 0) {
		return kParserTokenGreaterEq;
	} else if (strcmp(_currentTokenStr, "->") == 0) {
		return kParserTokenThen;
	} else if (strcmp(_currentTokenStr, "End") == 0) {
		return kParserTokenEnd;
	} else if (strcmp(_currentTokenStr, "Movies") == 0) {
		return kParserTokenMovies;
	} else if (strcmp(_currentTokenStr, "Scene") == 0) {
		return kParserTokenScene;
	} else if (strcmp(_currentTokenStr, "Screen") == 0) {
		return kParserTokenScreen;
	} else if (strcmp(_currentTokenStr, "Midi") == 0) {
		return kParserTokenMidi;
	} else if (strcmp(_currentTokenStr, "NULL") == 0) {
		return kParserTokenNULL;
	} else if (strcmp(_currentTokenStr, "Object:") == 0) {
		return kParserTokenObject;
	} else if (strcmp(_currentTokenStr, "IfNewObject:") == 0) {
		return kParserTokenIfNewObject;
	} else if (strcmp(_currentTokenStr, "Bag") == 0) {
		return kParserTokenBag;
	} else if (strcmp(_currentTokenStr, "Box") == 0) {
		return kParserTokenBox;
	} else if (strcmp(_currentTokenStr, "BagEnd") == 0) {
		return kParserTokenBagEnd;
	} else if (strcmp(_currentTokenStr, "MoviesEnd") == 0) {
		return kParserTokenMoviesEnd;
	} else if (strcmp(_currentTokenStr, "SceneEnd") == 0) {
		return kParserTokenSceneEnd;
	} else if (strcmp(_currentTokenStr, "ObjectEnd") == 0) {
		return kParserTokenObjectEnd;
	} else if (strcmp(_currentTokenStr, "BoxEnd") == 0) {
		return kParserTokenBoxEnd;
	} else if (strcmp(_currentTokenStr, "Class") == 0) {
		return kParserTokenClass;
	} else if (strcmp(_currentTokenStr, "Memory") == 0) {
		return kParserTokenMemory;
	} else if (strcmp(_currentTokenStr, "Coord") == 0) {
		return kParserTokenCoord;
	} else if (strcmp(_currentTokenStr, "AddCoordX") == 0) {
		return kParserTokenAddCoordX;
	} else if (strcmp(_currentTokenStr, "AddCoordY") == 0) {
		return kParserTokenAddCoordY;
	} else if (strcmp(_currentTokenStr, "CoordX") == 0) {
		return kParserTokenCoordX;
	} else if (strcmp(_currentTokenStr, "CoordY") == 0) {
		return kParserTokenCoordY;
	} else if (strcmp(_currentTokenStr, "Depth") == 0) {
		return kParserTokenDepth;
	} else if (strcmp(_currentTokenStr, "Move") == 0) {
		return kParserTokenMove;
	} else if (strcmp(_currentTokenStr, "Cel") == 0) {
		return kParserTokenCel;
	} else if (strcmp(_currentTokenStr, "Mirror") == 0) {
		return kParserTokenMirror;
	} else if (strcmp(_currentTokenStr, "No") == 0) {
		return kParserTokenNo;
	} else if (strcmp(_currentTokenStr, "X") == 0) {
		return kParserTokenX;
	} else if (strcmp(_currentTokenStr, "Y") == 0) {
		return kParserTokenY;
	} else if (strcmp(_currentTokenStr, "XY") == 0) {
		return kParserTokenXY;
	} else if (strcmp(_currentTokenStr, "Init") == 0) {
		return kParserTokenInit;
	} else if (strcmp(_currentTokenStr, "NoInit") == 0) {
		return kParserTokenNoInit;
	} else if (strcmp(_currentTokenStr, "Simple") == 0) {
		return kParserTokenSimple;
	} else if (strcmp(_currentTokenStr, "Random") == 0) {
		return kParserTokenRandom;
	} else if (strcmp(_currentTokenStr, "Put") == 0) {
		return kParserTokenPut;
	} else if (strcmp(_currentTokenStr, "LoadStatus") == 0) {
		return kParserTokenLoadStatus;
	} else if (strcmp(_currentTokenStr, "Disable") == 0) {
		return kParserTokenDisable;
	} else if (strcmp(_currentTokenStr, "Enable") == 0) {
		return kParserTokenEnable;
	} else if (strcmp(_currentTokenStr, "Mix") == 0) {
		return kParserTokenMix;
	} else if (strcmp(_currentTokenStr, "ScenenNumber") == 0) { // C1_07.SCN
		return kParserTokenSceneNumber;
	} else {
		return kParserTokenUnknown;
	}
}

static void getToken_Int(int *i) {
	if (_currentTokenStr) {
		errno = 0;
		*i = strtol(_currentTokenStr, 0, 0);
		if (errno == 0) {
			return;
		}
	}
	error("Parse error for integer '%s'", !_currentTokenStr ? "" : _currentTokenStr);
}

static void getNextToken_Int(char **s, int *i) {
	_currentTokenStr = stringNextToken(s);
	getToken_Int(i);
}

static void getNextToken_ArrayIndex(char **s, int *i) {
	_currentTokenStr = stringNextToken(s);
	if (_currentTokenStr && _currentTokenStr[0] == '[') {
		errno = 0;
		*i = strtol(&_currentTokenStr[1], 0, 0);
		if (errno == 0) {
			*i -= 1;
			return;
		}
	}
	error("Parse error for array index '%s'", !_currentTokenStr ? "" : _currentTokenStr);
}

static void getNextToken_Coord(char **s, int16_t *i, int16_t *j) {
	_currentTokenStr = stringTrimLeft(*s);
	if (_currentTokenStr && _currentTokenStr[0] == '(') {
		errno = 0;
		*i = strtol(&_currentTokenStr[1], 0, 0);
		if (errno == 0) {
			const char *subTokenStr = strchr(&_currentTokenStr[1], ',');
			if (subTokenStr) {
				errno = 0;
				*j = strtol(&subTokenStr[1], 0, 0);
				if (errno == 0) {
					const char *endTokenStr = strchr(&subTokenStr[1], ')');
					if (endTokenStr) {
						*s = (char *)&endTokenStr[1];
						return;
					}
				}
			}
		}
	}
	error("Parse error for coord '%s'", !_currentTokenStr ? "" : _currentTokenStr);
}

static bool compareOp(int value1, int value2, int op) {
	bool compareResult = false;
	switch (op) {
	case kParserTokenEq:
		compareResult = (value1 == value2);
		break;
	case kParserTokenNotEq:
		compareResult = (value1 != value2);
		break;
	case kParserTokenLower:
		compareResult = (value1 < value2);
		break;
	case kParserTokenLowerEq:
		compareResult = (value1 <= value2);
		break;
	case kParserTokenGreater:
		compareResult = (value1 > value2);
		break;
	case kParserTokenGreaterEq:
		compareResult = (value1 >= value2);
		break;
	default:
		error("Invalid compare operator %d", op);
		break;
	}
	return compareResult;
}

static bool parseToken_GlobalMemory(char **s, Game *g) {
	int var, testValue, op;

	getNextToken_ArrayIndex(s, &var);
	op = getNextToken(s);
	_currentToken = getNextToken(s);
	if (_currentToken == kParserTokenSceneNumber) {
		testValue = g->_sceneNumber;
	} else {
		getToken_Int(&testValue);
	}
	assert(var >= 0 && var < Game::NUM_VARS);
	return compareOp(g->_varsTable[var], testValue, op);
}

static void parseToken_Screen(char **s, Game *g) {
	for (int i = 0; i < 10; ++i) {
		g->_boxesCountTable[i] = 0;
	}
	_currentTokenStr = stringNextToken(s);
	g->loadWGP(_currentTokenStr);
	strcpy(g->_currentSceneWgp, _currentTokenStr);
	g->_loadDataState = (g->_sceneObjectsCount == 0) ? 1 : 2;
}

static void parseToken_Midi(char **s, Game *g) {
	g->_musicTrack = 0;
	_currentToken = getNextToken(s);
	if (_currentToken == kParserTokenNULL) {
		g->stopMusic();
	} else {
		if (strcmp(_currentTokenStr, g->_musicName) != 0) {
			g->playMusic(_currentTokenStr);
		}
		strcpy(g->_musicName, _currentTokenStr);
		getNextToken_Int(s, &g->_musicTrack);
	}
}

static void parseToken_Object(char **s, Game *g) {
	_currentTokenStr = stringNextToken(s);
	_currentSceneObject = 0;
	for (int i = 0; i < g->_sceneObjectsCount; ++i) {
		if (strcasecmp(g->_sceneObjectsTable[i].name, _currentTokenStr) == 0) {
			_currentSceneObject = &g->_sceneObjectsTable[i];
			break;
		}
	}
	if (_currentState == kParserStateObject) {
		if (_currentSceneObject) {
			memset(_currentSceneObject->varsTable, 0, sizeof(_currentSceneObject->varsTable));
			_currentSceneObject->state = 0;
			// FIXME: fixes _08.SCN TELE05.SCN transition + IARD obj
			_currentSceneObject->statePrev = 0;
		}
	}
}

static void parseToken_Bag(char **s, Game *g) {
	getNextToken_Int(s, &g->_bagPosX);
	getNextToken_Int(s, &g->_bagPosY);
}

static void parse_Object(char **s, Game *g) {
	int var, index, value;
	SceneObjectStatus *sos;

	if (!_currentSceneObject) {
		stringNextTokenEOL(s);
		return;
	}
	switch (_currentToken) {
	case kParserTokenClass:
		_currentTokenStr = stringNextToken(s);
		strcpy(_currentSceneObject->className, _currentTokenStr);
		break;
	case kParserTokenMemory:
		getNextToken_ArrayIndex(s, &var);
		getNextToken_Int(s, &value);
		assert(var >= 0 && var < 10);
		_currentSceneObject->varsTable[var] = value;
		break;
	case kParserTokenCoord:
		getNextToken_Coord(s, &_currentSceneObject->xInit, &_currentSceneObject->yInit);
		break;
	case kParserTokenAddCoordX:
		getNextToken_Int(s, &value);
		_currentSceneObject->xInit += value;
		break;
	case kParserTokenAddCoordY:
		getNextToken_Int(s, &value);
		_currentSceneObject->yInit += value;
		break;
	case kParserTokenCoordX:
		getNextToken_Int(s, &value);
		_currentSceneObject->xInit = value;
		break;
	case kParserTokenCoordY:
		getNextToken_Int(s, &value);
		_currentSceneObject->yInit = value;
		break;
	case kParserTokenDepth:
		getNextToken_Int(s, &value);
		_currentSceneObject->zInit = value;
		break;
	case kParserTokenMove:
		getNextToken_Int(s, &value);
		_currentSceneObject->motionNum = value - 1;
		_currentSceneObject->motionFrameNum = 0;
		break;
	case kParserTokenCel:
		getNextToken_Int(s, &value);
		_currentSceneObject->motionNum = value - 1;
		getNextToken_Int(s, &value);
		_currentSceneObject->motionFrameNum = value - 1;
		break;
	case kParserTokenMirror:
		_currentToken = getNextToken(s);
		switch (_currentToken) {
		case kParserTokenNo:
			_currentSceneObject->flipInit = 0;
			break;
		case kParserTokenX:
			_currentSceneObject->flipInit = 1;
			break;
		case kParserTokenY:
			_currentSceneObject->flipInit = 2;
			break;
		case kParserTokenXY:
			_currentSceneObject->flipInit = 3;
			break;
		default:
			error("Unexpected mirror mode %s", _currentTokenStr);
			break;
		}
		break;
	case kParserTokenInit:
		_currentToken = getNextToken(s);
		switch (_currentToken) {
		case kParserTokenNoInit:
			_currentSceneObject->mode = 0;
			break;
		case kParserTokenSimple:
			_currentSceneObject->mode = 1;
			break;
		case kParserTokenRandom:
			_currentSceneObject->mode = 2;
			getNextToken_Int(s, &value);
			_currentSceneObject->modeRndMul = value;
			break;
		case kParserTokenPut:
			_currentSceneObject->mode = 3;
			break;
		default:
			error("Unexpected init mode %s", _currentTokenStr);
			break;
		}
		break;
	case kParserTokenLoadStatus:
		getNextToken_ArrayIndex(s, &index);
		sos = g->derefSceneObjectStatus(index);
		_currentSceneObject->xInit = sos->x;
		_currentSceneObject->yInit = sos->y;
		_currentSceneObject->zInit = sos->z;
		_currentSceneObject->motionNum = sos->motionNum;
		_currentSceneObject->motionFrameNum = sos->frameNum;
		_currentSceneObject->flipInit = sos->flip;
		break;
	default:
		error("Unexpected token %d state %d", _currentToken, _currentState);
		break;
	}
}

static void parse_BagObject(char **s, Game *g) {
	if (g->findBagObjectByName(_currentTokenStr) == -1) {
		assert(g->_bagObjectsCount < Game::NUM_BAG_OBJECTS);
		BagObject *bo = &g->_bagObjectsTable[g->_bagObjectsCount];
		strcpy(bo->name, _currentTokenStr);
		_currentTokenStr = stringNextToken(s);
		bo->data = g->loadFile(_currentTokenStr, 0, &bo->dataSize);
		++g->_bagObjectsCount;
	} else {
		stringNextTokenEOL(s);
	}
}

static void parse_SceneCondition(char **s, Game *g) {
	int num;

	getToken_Int(&num);
	assert(g->_sceneConditionsCount < Game::NUM_NEXT_SCENES);
	NextScene *ns = &g->_nextScenesTable[g->_sceneConditionsCount];
	ns->num = num;
	_currentTokenStr = stringNextToken(s);
	strcpy(ns->name, _currentTokenStr);
	stringToUpperCase(ns->name);
	++g->_sceneConditionsCount;
}

static void parse_BoxDescription(char **s, Game *g) {
	int box, value;

	getToken_Int(&box);
	--box;
	// 10 Mix -1 192 64 (582,332) (640,480)
	Box *b = g->derefBox(box, g->_boxesCountTable[box]);
	_currentToken = getNextToken(s);
	switch (_currentToken) {
	case kParserTokenDisable:
		b->state = 0;
		break;
	case kParserTokenEnable:
		b->state = 1;
		break;
	case kParserTokenMix:
		b->state = 2;
		getNextToken_Int(s, &value);
		b->z = value;
		getNextToken_Int(s, &value);
		b->startColor = value;
		getNextToken_Int(s, &value);
		b->endColor = value;
		assert(b->startColor + b->endColor <= 256); // invalid box mix color
		break;
	default:
		error("Unexpected token %d state %d", _currentToken, _currentState);
		break;
	}
	getNextToken_Coord(s, &b->x1, &b->y1);
	getNextToken_Coord(s, &b->x2, &b->y2);
	++g->_boxesCountTable[box];
}

void Game::parseSCN(const char *fileName) {
	debug(DBG_GAME, "Game::parseSCN()");

	FileHolder fp(_fs, fileName);
	_sceneDescriptionSize = fp->size();
	_sceneDescriptionBuffer = (char *)malloc(_sceneDescriptionSize + 1);
	if (!_sceneDescriptionBuffer) {
		error("Unable to allocate %d bytes", _sceneDescriptionSize + 1);
	}
	fp->read(_sceneDescriptionBuffer, _sceneDescriptionSize);
	_sceneDescriptionBuffer[_sceneDescriptionSize] = 0;
	stringStripComments(_sceneDescriptionBuffer);

	int anim = 0;
	bool loadMovData = false;
	_sceneNumber = 0;
	_currentSceneObject = 0;
	memcpy(_defaultVarsTable, _varsTable, sizeof(_varsTable));

	_currentState = kParserStateDef;
	_stopParsing = false;
	for (char *s = _sceneDescriptionBuffer; !_stopParsing && s; ) {
		bool didTest = false;
		bool compareTest = true;
		while ((_currentToken = getNextToken(&s)) == kParserTokenGlobalMemory) {
			compareTest = compareTest && parseToken_GlobalMemory(&s, this);
			didTest = true;
		}
		if (didTest) {
			if (!compareTest) {
				// condition statement is false, skip to next line
				stringNextTokenEOL(&s);
				continue;
			}
			if (_currentToken != kParserTokenThen) {
				error("Unexpected token %d", _currentToken);
			}
			_currentToken = getNextToken(&s);
		}
		if (_currentState != kParserStateDef) {
			switch (_currentState) {
			case kParserStateMovies:
				if (_currentToken == kParserTokenMoviesEnd) {
					if (!loadMovData) {
						clearSceneData(anim - 1);
					}
					_currentState = kParserStateDef;
				} else {
					if (!loadMovData) {
						if (anim < _animationsCount && strcasecmp(_animationsTable[anim].name, _currentTokenStr) == 0) {
							++anim;
						} else {
							if (_animationsCount != 0) {
								clearSceneData(anim - 1);
							}
							_loadDataState = 1;
							loadMovData = true;
						}
					}
					if (loadMovData) {
						loadMOV(_currentTokenStr);
					}
				}
				break;
			case kParserStateBag:
				if (_currentToken == kParserTokenBagEnd) {
					_currentState = kParserStateDef;
				} else {
					parse_BagObject(&s, this);
				}
				break;
			case kParserStateScene:
				if (_currentToken == kParserTokenSceneEnd) {
					_currentState = kParserStateDef;
				} else {
					parse_SceneCondition(&s, this);
				}
				break;
			case kParserStateObject:
				if (_currentToken == kParserTokenObjectEnd) {
					_currentState = kParserStateDef;
				} else {
					parse_Object(&s, this);
				}
				break;
			case kParserStateNewObject:
				if (_currentToken == kParserTokenObjectEnd) {
					_currentState = kParserStateDef;
				} else {
					if (_currentSceneObject) {
						if (_sceneObjectMotionsTable[_currentSceneObject->motionInit].animNum < anim) {
							_currentSceneObject = 0;
						}
					}
					parse_Object(&s, this);
				}
				break;
			case kParserStateBox:
				if (_currentToken == kParserTokenBoxEnd) {
					_currentState = kParserStateDef;
				} else {
					parse_BoxDescription(&s, this);
				}
				break;
			}
		} else {
			switch (_currentToken) {
			case kParserTokenEnd:
				_stopParsing = true;
				break;
			case kParserTokenMovies:
				_currentState = kParserStateMovies;
				break;
			case kParserTokenScene:
				_currentState = kParserStateScene;
				break;
			case kParserTokenScreen:
				parseToken_Screen(&s, this);
				break;
			case kParserTokenSceneNumber:
				getNextToken_Int(&s, &_sceneNumber);
				break;
			case kParserTokenMidi:
				parseToken_Midi(&s, this);
				break;
			case kParserTokenObject:
				_currentState = kParserStateObject;
				parseToken_Object(&s, this);
				break;
			case kParserTokenIfNewObject:
				_currentState = kParserStateNewObject;
				parseToken_Object(&s, this);
				break;
			case kParserTokenBag:
				_currentState = kParserStateBag;
				parseToken_Bag(&s, this);
				break;
			case kParserTokenBox:
				_currentState = kParserStateBox;
				break;
			default:
				error("Unexpected token %d state %d", _currentToken, _currentState);
				break;
			}
		}
	}

	free(_sceneDescriptionBuffer);
	_sceneDescriptionBuffer = 0;
	_sceneDescriptionSize = 0;
}
