/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "game.h"
#include "decoder.h"
#include "systemstub.h"

bool Game::executeConditionOpcode(int num) {
	switch (num) {
	case    10: return cop_true();
	case   100: return cop_isInRandomRange();
	case   500: return cop_isKeyPressed();
	case   510: return cop_isKeyNotPressed();
	case  1100: return cop_testMouseButtons();
	case  2500: return cop_isObjectInScene();
	case  3000: return cop_testObjectPrevState();
	case  3010: return cop_testObjectState();
	case  3050: return cop_isObjectInRect();
	case  3100: return cop_testPrevObjectTransformXPos();
	case  3105: return cop_testObjectTransformXPos();
	case  3110: return cop_testPrevObjectTransformYPos();
	case  3150: return cop_testObjectTransformYPos();
	case  3300: return cop_testObjectPrevFlip();
	case  3310: return cop_testObjectFlip();
	case  3400: return cop_testObjectPrevFrameNum();
	case  3410: return cop_testObjectFrameNum();
	case  3500: return cop_testPrevMotionNum();
	case  3510: return cop_testMotionNum();
	case  3600: return cop_testObjectVar();
	case  3700: return cop_testObjectAndObjectXPos();
	case  3710: return cop_testObjectAndObjectYPos();
	case  4110: return cop_testObjectMotionYPos();
	case  6000: return cop_testVar();
	case  6500: return cop_isCurrentBagAction();
	case  7000: return cop_isObjectInBox();
	case  7500: return cop_isObjectNotInBox();
	case  8500: return cop_isObjectNotIntersectingBox();
	case 10000: return cop_isCurrentBagObject();
	case 20000: return cop_isLifeBarDisplayed();
	case 20010: return cop_isLifeBarNotDisplayed();
	case 25000: return cop_testLastDialogue();
	case 30000: return cop_isNextScene();
	default:
		error("Invalid condition %d", num);
	}
	return false;
}

void Game::executeOperatorOpcode(int num) {
	switch (num) {
	case  3000: return oop_initializeObject();
	case  3100: return oop_evalCurrentObjectX();
	case  3110: return oop_evalCurrentObjectY();
	case  3120: return oop_evalObjectX();
	case  3130: return oop_evalObjectY();
	case  3200: return oop_evalObjectZ();
	case  3300: return oop_setObjectFlip();
	case  3400: return oop_adjustObjectPos_vv0000();
	case  3410: return oop_adjustObjectPos_vv1v00();
	case  3430: return oop_adjustObjectPos_vv1v1v();
	case  3440: return oop_setupObjectPos_121();
	case  3460: return oop_setupObjectPos_122();
	case  3480: return oop_setupObjectPos_123();
	case  3500: return oop_adjustObjectPos_1v0000();
	case  3530: return oop_adjustObjectPos_1v1v1v();
	case  3540: return oop_setupObjectPos_021();
	case  3560: return oop_setupObjectPos_022();
	case  3580: return oop_setupObjectPos_023();
	case  4000: return oop_evalObjectVar();
	case  4100: return oop_translateObjectXPos();
	case  4200: return oop_translateObjectYPos();
	case  5000: return oop_setObjectMode();
	case  5100: return oop_setObjectInitPos();
	case  5110: return oop_setObjectTransformInitPos();
	case  5112: return oop_evalObjectXInit();
	case  5114: return oop_evalObjectYInit();
	case  5200: return oop_evalObjectZInit();
	case  5300: return oop_setObjectFlipInit();
	case  5400: return oop_setObjectCel();
	case  5500: return oop_resetObjectCel();
	case  6000: return oop_evalVar();
	case  6100: return oop_getSceneNumberInVar();
	case  7000: return oop_disableBox();
	case  7010: return oop_enableBox();
	case  7100: return oop_evalBoxesXPos();
	case  7110: return oop_evalBoxesYPos();
	case  7200: return oop_setBoxToObject();
	case  7300: return oop_clipBoxes();
	case  8000: return oop_saveObjectStatus();
	case 10000: return oop_addObjectToBag();
	case 11000: return oop_removeObjectFromBag();
	case 20000: return oop_playSoundLowerEqualPriority();
	case 20010: return oop_playSoundLowerPriority();
	case 25000: return oop_startDialogue();
	case 30000: return oop_switchSceneClearBoxes();
	case 30010: return oop_switchSceneCopyBoxes();
	default:
		error("Invalid operator %d", num);
	}
}

void Game::evalExpr(int16_t *val) {
	int16_t op = _objectScript.fetchNextWord();
	int16_t arg = _objectScript.fetchNextWord();
	switch (op) {
	case 0:
		*val = arg;
		break;
	case 1:
		*val += arg;
		break;
	case 2:
		*val -= arg;
		break;
	case 3:
		*val *= arg;
		break;
	case 4:
		*val /= arg;
		break;
	default:
		error("Invalid eval op %d", op);
		break;
	}
}

bool Game::testExpr(int16_t val) {
	bool ret = false;
	int16_t op = _objectScript.fetchNextWord();
	if (op == -1) {
		int count = _objectScript.fetchNextWord();
		while (count--) {
			int16_t cmp1 = _objectScript.fetchNextWord();
			int16_t cmp2 = _objectScript.fetchNextWord();
			if (cmp1 > cmp2) {
				warning("testExpr cmp %d,%d", cmp1, cmp2);
			}
			if (cmp1 <= val && cmp2 >= val) {
				ret = true;
			}
		}
	} else {
		int16_t arg = _objectScript.fetchNextWord();
		switch (op) {
		case 0:
			if (arg == val) {
				ret = true;
			}
			break;
		case 1:
			if (arg != val) {
				ret = true;
			}
			break;
		case 2:
			if (arg > val) {
				ret = true;
			}
			break;
		case 3:
			if (arg < val) {
				ret = true;
			}
			break;
		case 4:
			if (arg >= val) {
				ret = true;
			}
			break;
		case 5:
			if (arg <= val) {
				ret = true;
			}
			break;
		default:
			error("Invalid eval op %d", op);
			break;
		}
	}
	return ret;
}

bool Game::cop_true() {
	debug(DBG_OPCODES, "Game::cop_true");
	return true;
}

bool Game::cop_isInRandomRange() {
	debug(DBG_OPCODES, "Game::cop_isInRandomRange");
	int16_t rnd = _rnd.getNumber();
	int t = (int16_t)_objectScript.fetchNextWord();
	return ((((t * rnd) / 0x8000) & 0xFFFF) == 0);
}

bool Game::cop_isKeyPressed() {
	debug(DBG_OPCODES, "Game::cop_isKeyPressed");
	int key = _objectScript.fetchNextWord();
	return _keysPressed[key] != 0;
}

bool Game::cop_isKeyNotPressed() {
	debug(DBG_OPCODES, "Game::cop_isKeyNotPressed");
	int key = _objectScript.fetchNextWord();
	return _keysPressed[key] == 0;
}

bool Game::cop_testMouseButtons() {
	debug(DBG_OPCODES, "Game::cop_testMouseButtons");
	bool ret = true;
	int t = _objectScript.fetchNextWord();
	switch (t) {
	case 0:
		ret = (_mouseButtonsPressed & kLeftMouseButton) != 0;
		break;
	case 1:
		ret = (_mouseButtonsPressed & kRightMouseButton) != 0;
		break;
	case 2:
		ret = (_mouseButtonsPressed & kLeftMouseButton) == 0;
		break;
	case 3:
		ret = (_mouseButtonsPressed & kRightMouseButton) == 0;
		break;
	}
	return ret;
}

bool Game::cop_isObjectInScene() {
	debug(DBG_OPCODES, "Game::cop_isObjectInScene");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		if (index != _objectScript.currentObjectNum) {
			ret = false;
		}
	}
	return ret;
}

bool Game::cop_testObjectPrevState() {
	debug(DBG_OPCODES, "Game::cop_testObjectPrevState");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		int16_t state = _objectScript.fetchNextWord();
		if (so->statePrev != state) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 2;
	}
	return ret;
}

bool Game::cop_testObjectState() {
	debug(DBG_OPCODES, "Game::cop_testObjectState");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		int16_t state = _objectScript.fetchNextWord();
		if (so->state != state) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 2;
	}
	return ret;
}

bool Game::cop_isObjectInRect() {
	debug(DBG_OPCODES, "Game::cop_isObjectInRect");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	int16_t var1E = _objectScript.fetchNextWord(); // x1
	int16_t var22 = _objectScript.fetchNextWord(); // y1
	int16_t var20 = _objectScript.fetchNextWord(); // x2
	int16_t var24 = _objectScript.fetchNextWord(); // y2
	assert(var1E <= var20);
	assert(var22 <= var24);
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->state != 0) {
			int xObj = so->xPrev + _sceneObjectFramesTable[so->frameNumPrev].hdr.w;
			if (xObj >= var1E && so->xPrev <= var20) {
				int yObj = so->yPrev + _sceneObjectFramesTable[so->frameNumPrev].hdr.h;
				if (yObj >= var22 && so->yPrev <= var24) {
					return ret;
				}
			}
		}
		ret = false;
	}
	return ret;
}

bool Game::cop_testPrevObjectTransformXPos() {
	debug(DBG_OPCODES, "Game::cop_testPrevObjectTransformXPos");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret && derefSceneObject(index)->statePrev != 0) {
		if (!comparePrevObjectTransformXPos(index)) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 14;
	}
	return ret;
}

bool Game::cop_testObjectTransformXPos() {
	debug(DBG_OPCODES, "Game::cop_testObjectTransformXPos");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret && derefSceneObject(index)->state != 0) {
		if (!compareObjectTransformXPos(index)) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 14;
	}
	return ret;
}

bool Game::cop_testPrevObjectTransformYPos() {
	debug(DBG_OPCODES, "Game::cop_testPrevObjectTransformYPos");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret && derefSceneObject(index)->statePrev != 0) {
		if (!comparePrevObjectTransformYPos(index)) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 14;
	}
	return ret;

}

bool Game::cop_testObjectTransformYPos() {
	debug(DBG_OPCODES, "Game::cop_testObjectTransformYPos");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret && derefSceneObject(index)->state != 0) {
		if (!compareObjectTransformYPos(index)) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 14;
	}
	return ret;
}

bool Game::cop_testObjectPrevFlip() {
	debug(DBG_OPCODES, "Game::cop_testObjectPrevFlip");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		int16_t flip = _objectScript.fetchNextWord();
		if (flip != so->flipPrev) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 2;
	}
	return ret;
}

bool Game::cop_testObjectFlip() {
	debug(DBG_OPCODES, "Game::cop_testObjectFlip");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		int16_t flip = _objectScript.fetchNextWord();
		if (flip != so->flip) {
			ret = false;
		}
	} else {
		_objectScript.dataOffset += 2;
	}
	return ret;
}

bool Game::cop_testObjectPrevFrameNum() {
	debug(DBG_OPCODES, "Game::cop_testObjectPrevFrameNum");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t val = so->frameNumPrev - _sceneObjectMotionsTable[so->motionNum1].firstFrameIndex + 1;
			if (testExpr(val)) {
				return true;
			}
		}
		ret = false;
	}
	return ret;
}

bool Game::cop_testObjectFrameNum() {
	debug(DBG_OPCODES, "Game::cop_testObjectFrameNum");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t val = so->frameNum - _sceneObjectMotionsTable[so->motionNum2].firstFrameIndex + 1;
			if (testExpr(val)) {
				return true;
			}
		}
		ret = false;
	}
	return ret;
}

bool Game::cop_testPrevMotionNum() {
	debug(DBG_OPCODES, "Game::cop_testPrevMotionNum");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t _dx = so->motionNum1 - _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstMotionIndex;
			int16_t _ax = 0;
			if (_objectScript.objectFound) {
				if (_sceneObjectMotionsTable[so->motionNum1].animNum != _sceneObjectMotionsTable[so->motionInit].animNum) {
					_ax = _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].unk26;
				}
			}
			if (testExpr(_ax + _dx + 1)) {
				return true;
			}
		}
		ret = false;
	}
	return ret;
}

bool Game::cop_testMotionNum() {
	debug(DBG_OPCODES, "Game::cop_testMotionNum");
	bool ret = true;
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t _dx = so->motionNum2 - _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstMotionIndex;
			int16_t _ax = 0;
			if (_objectScript.objectFound) {
				if (_sceneObjectMotionsTable[so->motionNum1].animNum != _sceneObjectMotionsTable[so->motionInit].animNum) {
					_ax = _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].unk26;
				}
			}
			if (testExpr(_ax + _dx + 1)) {
				return true;
			}
		}
		ret = false;
	}
	return ret;
}

bool Game::cop_testObjectVar() {
	debug(DBG_OPCODES, "Game::cop_testObjectVar()");
	bool ret = true;
	int var = _objectScript.fetchNextWord();
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		assert(var >= 0 && var < 10);
		if (!testExpr(so->varsTable[var])) {
			ret = false;
		}
	}
	return ret;
}

bool Game::cop_testObjectAndObjectXPos() {
	debug(DBG_OPCODES, "Game::cop_testObjectAndObjectXPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t var1E = getObjectTransformXPos(index);
			int16_t var20 = getObjectTransformXPos(index);
			int var18 = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
			if (var18 != -1) {
				so = derefSceneObject(var18);
				if (so->statePrev == 0) {
					return false;
				}
				int16_t var22 = getObjectTransformXPos(var18);
				int16_t var24 = getObjectTransformXPos(var18);
				int16_t _dx = MIN(var1E, var20);
				int16_t _ax = MAX(var22, var24);
				if (_dx <= _ax) {
					_dx = MAX(var1E, var20);
					_ax = MIN(var22, var24);
					if (_dx >= _ax) {
						return true;
					}
				}
			}
			return false;
		}
	}
	return false;
}

bool Game::cop_testObjectAndObjectYPos() {
	debug(DBG_OPCODES, "Game::cop_testObjectAndObjectYPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t var22 = getObjectTransformYPos(index);
			int16_t var24 = getObjectTransformYPos(index);
			int var18 = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
			if (var18 != -1) {
				so = derefSceneObject(var18);
				if (so->statePrev == 0) {
					return false;
				}
				int16_t var1E = getObjectTransformYPos(var18);
				int16_t var20 = getObjectTransformYPos(var18);
				int16_t _dx = MIN(var22, var24);
				int16_t _ax = MAX(var1E, var20);
				if (_dx <= _ax) {
					_dx = MAX(var22, var24);
					_ax = MIN(var1E, var20);
					if (_dx >= _ax) {
						return true;
					}
				}
			}
			return false;
		}
	}
	return false;
}

bool Game::cop_testObjectMotionYPos() {
	debug(DBG_OPCODES, "Game::cop_testObjectMotionYPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		int num = so->motionNum + so->motionInit;
		int16_t var1A = _sceneObjectMotionsTable[num].firstFrameIndex + so->motionFrameNum;
		int _ax = so->yPrev - so->yInit;
		_ax -= _sceneObjectFramesTable[so->frameNumPrev].hdr.yPos;
		_ax += _sceneObjectFramesTable[var1A].hdr.yPos;
		int16_t div = _objectScript.fetchNextWord();
		var1A = _ax % div;
		if (var1A < 0) {
			var1A += div;
		}
		int16_t cmp = _objectScript.fetchNextWord();
		if (var1A == cmp && so->state == 1) {
			return true;
		}
	} else {
		_objectScript.dataOffset += 4;
	}
	return false;
}

bool Game::cop_testVar() {
	debug(DBG_OPCODES, "Game::cop_testVar()");
	int var = _objectScript.fetchNextWord();
	bool ret = testExpr(_varsTable[var]);
	return ret;
}

bool Game::cop_isCurrentBagAction() {
	debug(DBG_OPCODES, "Game::cop_isCurrentBagAction()");
	int16_t num = _objectScript.fetchNextWord();
	return num == _currentBagAction;
}

bool Game::cop_isObjectInBox() {
	debug(DBG_OPCODES, "Game::cop_isObjectInBox()");
	bool ret = true;
	int16_t var1A = _objectScript.fetchNextWord(); // boxNum
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound); // var18
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t var1E = getObjectTransformXPos(index); // x1
			int16_t var20 = getObjectTransformXPos(index); // x2
			int16_t var22 = getObjectTransformYPos(index); // y1
			int16_t var24 = getObjectTransformYPos(index); // y2
			bool foundBox = false;
			for (int i = 0; i < _boxesCountTable[var1A]; ++i) {
				Box *box = derefBox(var1A, i);
				if (boxInRect(box, var1E, var20, var22, var24) && box->state == 1) {
					foundBox = true;
					break;
				}
			}
			if (foundBox) {
				return true;
			}
			foundBox = false;
			for (int i = 0; i < _boxesCountTable[10 + var1A]; ++i) {
				Box *box = derefBox(10 + var1A, i);
				if (boxInRect(box, var1E, var20, var22, var24) && box->state == 1) {
					foundBox = true;
					break;
				}
			}
			if (foundBox) {
				return true;
			}
		}
	}
	return false;
}

bool Game::cop_isObjectNotInBox() {
	debug(DBG_OPCODES, "Game::cop_isObjectNotInBox()");
	bool ret = true;
	int16_t var1A = _objectScript.fetchNextWord(); // boxNum
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound); // var18
	if (index == -1) {
		ret = false;
	}
	if (ret) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t var1E = getObjectTransformXPos(index);
			int16_t var20 = getObjectTransformXPos(index);
			int16_t var22 = getObjectTransformYPos(index);
			int16_t var24 = getObjectTransformYPos(index);
			bool foundBox = false;
			for (int i = 0; i < _boxesCountTable[var1A]; ++i) {
				Box *box = derefBox(var1A, i);
				if (boxInRect(box, var1E, var20, var22, var24) && box->state == 1) {
					foundBox = true;
					break;
				}
			}
			if (foundBox) {
				return false;
			}
			foundBox = false;
			for (int i = 0; i < _boxesCountTable[10 + var1A]; ++i) {
				Box *box = derefBox(10 + var1A, i);
				if (boxInRect(box, var1E, var20, var22, var24) && box->state == 1) {
					foundBox = true;
					break;
				}
			}
			if (foundBox) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool Game::cop_isObjectNotIntersectingBox() {
	debug(DBG_OPCODES, "Game::cop_isObjectNotIntersectingBox()");
	int16_t var1A = _objectScript.fetchNextWord();
	int var18 = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (var18 != -1) {
		SceneObject *so = derefSceneObject(var18);
		if (so->statePrev != 0) {
			int16_t var1E = getObjectTransformXPos(var18);
			int16_t var22 = getObjectTransformYPos(var18);
			var18 = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
			if (var18 != -1) {
				so = derefSceneObject(var18);
				if (so->statePrev != 0) {
					int16_t var20 = getObjectTransformXPos(var18);
					int16_t var24 = getObjectTransformYPos(var18);
					bool foundBox = false;
					for (int i = 0; i < _boxesCountTable[var1A]; ++i) {
						if (intersectsBox(var1A, i, var1E, var22, var20, var24)) {
							foundBox = true;
							break;
						}
					}
					if (foundBox) {
						return false;
					}
					foundBox = false;
					for (int i = 0; i < _boxesCountTable[10 + var1A]; ++i) {
						if (intersectsBox(10 + var1A, i, var1E, var22, var20, var24)) {
							foundBox = true;
							break;
						}
					}
					if (foundBox) {
						return false;
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool Game::cop_isCurrentBagObject() {
	debug(DBG_OPCODES, "Game::cop_isCurrentBagObject");
	const char *name = _objectScript.fetchNextString();
	int index = findBagObjectByName(name);
	return index != -1 && _currentBagObject == index;
}

bool Game::cop_isLifeBarDisplayed() {
	debug(DBG_OPCODES, "Game::cop_isLifeBarDisplayed");
	return _lifeBarDisplayed;
}

bool Game::cop_isLifeBarNotDisplayed() {
	debug(DBG_OPCODES, "Game::cop_isLifeBarNotDisplayed");
	return !_lifeBarDisplayed;
}

bool Game::cop_testLastDialogue() {
	debug(DBG_OPCODES, "Game::cop_testLastDialogue");
	return testExpr(_lastDialogueEndedId) && _dialogueEndedFlag != 0;
}

bool Game::cop_isNextScene() {
	debug(DBG_OPCODES, "Game::cop_isNextScene");
	int scene = _objectScript.fetchNextWord();
	for (int i = 0; i < _sceneConditionsCount; ++i) {
		if (_nextScenesTable[i].num == scene) {
			return true;
		}
	}
	return false;
}

void Game::oop_initializeObject() {
	debug(DBG_OPCODES, "Game::oop_initializeObject");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		int16_t op = _objectScript.fetchNextWord();
		if (op == 0) {
			if (so->state != 0) {
				so->x = so->xPrev;
				so->y = so->yPrev;
				so->frameNum = so->frameNumPrev;
				if (so->state == 2) {
					SceneObjectFrame *sof = derefSceneObjectFrame(so->frameNum);
					copyBufferToBuffer(so->x, _bitmapBuffer1.h + 1 - so->y - sof->hdr.h, sof->hdr.w, sof->hdr.h, &_bitmapBuffer3, &_bitmapBuffer1);
				}
				so->state = -1;
			}
		} else if (op == 1) {
			int16_t mode = so->mode;
			so->mode = 1;
			reinitializeObject(index);
			so->mode = mode;
			if (so->state == 2) {
				so->state = 1;
			}
		} else if (op == 2) {
			if (so->state == 1) {
				so->x = so->xPrev;
				so->y = so->yPrev;
				so->frameNum = so->frameNumPrev;
				so->state = 2;
			} else {
				int16_t mode = so->mode;
				so->mode = 3;
				reinitializeObject(index);
				so->mode = mode;
			}
		}
	} else {
		_objectScript.dataOffset += 2;
	}
}

void Game::oop_evalCurrentObjectX() {
	debug(DBG_OPCODES, "Game::oop_evalCurrentObjectX()");
	SceneObject *so = derefSceneObject(_objectScript.currentObjectNum);
	if (so->state != 0) {
		_objectScript.dataOffset += 6;
		evalExpr(&so->x);
	} else {
		_objectScript.dataOffset += 6 + 4;
	}
}

void Game::oop_evalCurrentObjectY() {
	debug(DBG_OPCODES, "Game::oop_evalCurrentObjectY()");
	SceneObject *so = derefSceneObject(_objectScript.currentObjectNum);
	if (so->state != 0) {
		_objectScript.dataOffset += 6;
		evalExpr(&so->y);
	} else {
		_objectScript.dataOffset += 6 + 4;
	}
}

void Game::oop_evalObjectX() {
	debug(DBG_OPCODES, "Game::oop_evalObjectX()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->x);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_evalObjectY() {
	debug(DBG_OPCODES, "Game::oop_evalObjectY()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->y);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_evalObjectZ() {
	debug(DBG_OPCODES, "Game::oop_evalObjectZ()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->z);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_setObjectFlip() {
	debug(DBG_OPCODES, "Game::oop_setObjectFlip()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->flip = _objectScript.fetchNextWord();
	} else {
		_objectScript.dataOffset += 2;
	}
}

void Game::oop_adjustObjectPos_vv0000() {
	debug(DBG_OPCODES, "Game::oop_adjustObjectPos_vv0000()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		int16_t a0 = _objectScript.fetchNextWord();
		int16_t a2 = _objectScript.fetchNextWord();
		changeObjectMotionFrame(_objectScript.currentObjectNum, index, _objectScript.objectFound, a2, a0, 0, 0, 0, 0);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_adjustObjectPos_vv1v00() {
	debug(DBG_OPCODES, "Game::oop_adjustObjectPos_vv1v00()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		int16_t a0 = _objectScript.fetchNextWord();
		int16_t a2 = _objectScript.fetchNextWord();
		int16_t a4 = _objectScript.fetchNextWord();
		_objectScript.fetchNextWord();
		changeObjectMotionFrame(_objectScript.currentObjectNum, index, _objectScript.objectFound, a2, a0, 1, a4, 0, 0);
	} else {
		_objectScript.dataOffset += 8;
	}
}

void Game::oop_adjustObjectPos_vv1v1v() {
	debug(DBG_OPCODES, "Game::oop_adjustObjectPos_vv1v1v()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		int16_t a0 = _objectScript.fetchNextWord();
		int16_t a2 = _objectScript.fetchNextWord();
		int16_t a4 = _objectScript.fetchNextWord();
		int16_t a6 = _objectScript.fetchNextWord();
		changeObjectMotionFrame(_objectScript.currentObjectNum, index, _objectScript.objectFound, a2, a0, 1, a4, 1, a6);
	} else {
		_objectScript.dataOffset += 8;
	}
}

void Game::oop_setupObjectPos_121() {
	debug(DBG_OPCODES, "Game::oop_setupObjectPos_121()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		setupObjectPos(_objectScript.currentObjectNum, index, _objectScript.objectFound, 1, 2, 1);
	} else {
		_objectScript.dataOffset += 16;
	}
}

void Game::oop_setupObjectPos_122() {
	debug(DBG_OPCODES, "Game::oop_setupObjectPos_122()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		setupObjectPos(_objectScript.currentObjectNum, index, _objectScript.objectFound, 1, 2, 2);
	} else {
		_objectScript.dataOffset += 28;
	}
}

void Game::oop_setupObjectPos_123() {
	debug(DBG_OPCODES, "Game::oop_setupObjectPos_123()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		setupObjectPos(_objectScript.currentObjectNum, index, _objectScript.objectFound, 1, 2, 3);
	} else {
		_objectScript.dataOffset += 20;
	}
}

void Game::oop_adjustObjectPos_1v0000() {
	debug(DBG_OPCODES, "Game::oop_adjustObjectPos_1v0000()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		int16_t a0 = _objectScript.fetchNextWord();
		changeObjectMotionFrame(_objectScript.currentObjectNum, index, _objectScript.objectFound, 1, a0, 0, 0, 0, 0);
	} else {
		_objectScript.dataOffset += 2;
	}
}

void Game::oop_adjustObjectPos_1v1v1v() {
	debug(DBG_OPCODES, "Game::oop_adjustObjectPos_1v1v1v()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		int16_t a0 = _objectScript.fetchNextWord();
		int16_t a2 = _objectScript.fetchNextWord();
		int16_t a4 = _objectScript.fetchNextWord();
		changeObjectMotionFrame(_objectScript.currentObjectNum, index, _objectScript.objectFound, 1, a0, 1, a2, 1, a4);
	} else {
		_objectScript.dataOffset += 6;
	}
}

void Game::oop_setupObjectPos_021() {
	debug(DBG_OPCODES, "Game::oop_setupObjectPos_021()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		setupObjectPos(_objectScript.currentObjectNum, index, _objectScript.objectFound, 0, 2, 1);
	} else {
		_objectScript.dataOffset += 14;
	}
}

void Game::oop_setupObjectPos_022() {
	debug(DBG_OPCODES, "Game::oop_setupObjectPos_022()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		setupObjectPos(_objectScript.currentObjectNum, index, _objectScript.objectFound, 0, 2, 2);
	} else {
		_objectScript.dataOffset += 26;
	}
}

void Game::oop_setupObjectPos_023() {
	debug(DBG_OPCODES, "Game::oop_setupObjectPos_023()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		setupObjectPos(_objectScript.currentObjectNum, index, _objectScript.objectFound, 0, 2, 3);
	} else {
		_objectScript.dataOffset += 18;
	}
}

void Game::oop_evalObjectVar() {
	debug(DBG_OPCODES, "Game::oop_evalObjectVar()");
	int var = _objectScript.fetchNextWord();
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		assert(var >= 0 && var < 10);
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->varsTable[var]);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_translateObjectXPos() {
	debug(DBG_OPCODES, "Game::oop_translateObjectXPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		int16_t a0 = _objectScript.fetchNextWord();
		int16_t a2 = _objectScript.fetchNextWord();
		int16_t a4 = _objectScript.fetchNextWord();
		int16_t a6 = _objectScript.fetchNextWord();
		int16_t var1A = getObjectTranslateXPos(index, a0, a2, a4);
		if (a2 / 2 >= var1A) {
			so->x -= MIN<int16_t>(a6, var1A);
		} else {
			so->x += MIN<int16_t>(a6, a2 - var1A);
		}
	} else {
		_objectScript.dataOffset += 8;
	}
}

void Game::oop_translateObjectYPos() {
	debug(DBG_OPCODES, "Game::oop_translateObjectYPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		int16_t a0 = _objectScript.fetchNextWord();
		int16_t a2 = _objectScript.fetchNextWord();
		int16_t a4 = _objectScript.fetchNextWord();
		int16_t a6 = _objectScript.fetchNextWord();
		int16_t var1A = getObjectTranslateYPos(index, a0, a2, a4);
		if (a2 / 2 >= var1A) {
			so->y -= MIN<int16_t>(a6, var1A);
		} else {
			so->y += MIN<int16_t>(a6, a2 - var1A);
		}
	} else {
		_objectScript.dataOffset += 8;
	}
}

void Game::oop_setObjectMode() {
	debug(DBG_OPCODES, "Game::oop_setObjectMode()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	int mode = _objectScript.fetchNextWord();
	int modeRndMul = 0;
	if (mode == 2) {
		modeRndMul = _objectScript.fetchNextWord();
	}
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->mode = mode;
		if (mode == 2) {
			so->modeRndMul = modeRndMul;
		}
	}
}

void Game::oop_setObjectInitPos() {
	debug(DBG_OPCODES, "Game::oop_setObjectInitPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->xInit = _objectScript.fetchNextWord();
		so->yInit = _objectScript.fetchNextWord();
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_setObjectTransformInitPos() {
	debug(DBG_OPCODES, "Game::oop_setObjectTransformInitPos()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->xInit = getObjectTransformXPos(_objectScript.currentObjectNum);
		so->yInit = getObjectTransformYPos(_objectScript.currentObjectNum);
	} else {
		_objectScript.dataOffset += 12;
	}
}

void Game::oop_evalObjectXInit() {
	debug(DBG_OPCODES, "Game::oop_evalObjectXInit()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->xInit);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_evalObjectYInit() {
	debug(DBG_OPCODES, "Game::oop_evalObjectYInit()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->yInit);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_evalObjectZInit() {
	debug(DBG_OPCODES, "Game::oop_evalObjectZInit()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		evalExpr(&so->zInit);
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_setObjectFlipInit() {
	debug(DBG_OPCODES, "Game::oop_setObjectFlipInit()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->flipInit = _objectScript.fetchNextWord();
	} else {
		_objectScript.dataOffset += 2;
	}
}

void Game::oop_setObjectCel() {
	debug(DBG_OPCODES, "Game::oop_setObjectCel()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->motionNum = _objectScript.fetchNextWord() - 1;
		so->motionFrameNum = _objectScript.fetchNextWord() - 1;
	} else {
		_objectScript.dataOffset += 4;
	}
}

void Game::oop_resetObjectCel() {
	debug(DBG_OPCODES, "Game::oop_resetObjectCel()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		so->motionNum = _objectScript.fetchNextWord() - 1;
		so->motionFrameNum = 0;
	} else {
		_objectScript.dataOffset += 2;
	}
}

void Game::oop_evalVar() {
	debug(DBG_OPCODES, "Game::oop_evalVar()");
	int var = _objectScript.fetchNextWord();
	assert(var >= 0 && var < NUM_VARS);
	evalExpr(&_varsTable[var]);
}

void Game::oop_getSceneNumberInVar() {
	debug(DBG_OPCODES, "Game::oop_getSceneNumberInVar()");
	int var = _objectScript.fetchNextWord();
	assert(var >= 0 && var < NUM_VARS);
	_varsTable[var] = _sceneNumber;
}

void Game::oop_disableBox() {
	debug(DBG_OPCODES, "Game::oop_disableBox()");
	int box = _objectScript.fetchNextWord();
	int index = _objectScript.fetchNextWord();
	// FIXME: workaround no box for using raft (C2_17.SCN, FLY*.SCN)
	if (_objectScript.currentObjectNum == 0 && (_objectScript.statementNum == 38 || _objectScript.statementNum == 39)) {
		return;
	}
	derefBox(box, index)->state = 0;
}

void Game::oop_enableBox() {
	debug(DBG_OPCODES, "Game::oop_enableBox()");
	int box = _objectScript.fetchNextWord();
	int index = _objectScript.fetchNextWord();
	derefBox(box, index)->state = 1;
}

void Game::oop_evalBoxesXPos() {
	debug(DBG_OPCODES, "Game::oop_evalBoxesXPos()");
	for (int b = 0; b < 10; ++b) {
		for (int i = 0; i < _boxesCountTable[b]; ++i) {
			Box *box = derefBox(b, i);
			evalExpr(&box->x1);
			_objectScript.dataOffset -= 4;
			evalExpr(&box->x2);
			_objectScript.dataOffset -= 4;
		}
	}
	_objectScript.dataOffset += 4;
}

void Game::oop_evalBoxesYPos() {
	debug(DBG_OPCODES, "Game::oop_evalBoxesYPos()");
	for (int b = 0; b < 10; ++b) {
		for (int i = 0; i < _boxesCountTable[b]; ++i) {
			Box *box = derefBox(b, i);
			evalExpr(&box->y1);
			_objectScript.dataOffset -= 4;
			evalExpr(&box->y2);
			_objectScript.dataOffset -= 4;
		}
	}
	_objectScript.dataOffset += 4;
}

void Game::oop_setBoxToObject() {
	debug(DBG_OPCODES, "Game::oop_setBoxToObject()");
	int16_t var1A = _objectScript.fetchNextWord();
	int16_t var1C = _objectScript.fetchNextWord();
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		if (so->statePrev != 0) {
			int16_t var1E = getObjectTransformXPos(index);
			int16_t var20 = getObjectTransformXPos(index);
			int16_t var22 = getObjectTransformYPos(index);
			int16_t var24 = getObjectTransformYPos(index);
			Box *box = derefBox(var1A, var1C);
			box->x1 = MIN(var1E, var20);
			box->x2 = MAX(var1E, var20);
			box->y1 = MIN(var24, var22);
			box->y2 = MAX(var24, var22);
			return;
		}
	}
	_objectScript.dataOffset += 24;
}

void Game::oop_clipBoxes() {
	debug(DBG_OPCODES, "Game::oop_clipBoxes()");
	int16_t x1 = _objectScript.fetchNextWord();
	int16_t y1 = _objectScript.fetchNextWord();
	int16_t x2 = _objectScript.fetchNextWord();
	int16_t y2 = _objectScript.fetchNextWord();
	for (int b = 0; b < 10; ++b) {
		for (int i = 0; i < _boxesCountTable[b]; ++i) {
			Box *box = derefBox(b, i);
			box->x1 = CLIP(box->x1, x1, x2);
			box->x2 = CLIP(box->x2, x1, x2);
			box->y1 = CLIP(box->y1, y1, y2);
			box->y2 = CLIP(box->y2, y1, y2);
		}
	}
}

void Game::oop_saveObjectStatus() {
	debug(DBG_OPCODES, "Game::oop_saveObjectStatus()");
	SceneObject *so = derefSceneObject(_objectScript.currentObjectNum);
	int xPrev;
	if (so->flipPrev == 2) {
		xPrev = so->xPrev + _sceneObjectFramesTable[so->frameNumPrev].hdr.w - 1;
	} else {
		xPrev = so->xPrev;
	}
	int index = _objectScript.fetchNextWord();
	SceneObjectStatus *stat = derefSceneObjectStatus(index);
	stat->x = xPrev;
	stat->y = so->yPrev;
	stat->z = so->zPrev;
	stat->motionNum = so->motionNum1 - _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstMotionIndex;
	stat->frameNum = so->frameNumPrev - _sceneObjectMotionsTable[so->motionNum1].firstFrameIndex;
	stat->flip = so->flipPrev;
}

void Game::oop_addObjectToBag() {
	debug(DBG_OPCODES, "Game::oop_addObjectToBag()");
	int index = findObjectByName(_objectScript.currentObjectNum, _objectScript.testObjectNum, &_objectScript.objectFound);
	if (index != -1) {
		SceneObject *so = derefSceneObject(index);
		if (findBagObjectByName(so->name) == -1) {
			assert(_bagObjectsCount < NUM_BAG_OBJECTS);
			BagObject *bo = &_bagObjectsTable[_bagObjectsCount];
			strcpy(bo->name, so->name);

			SceneObjectFrame *sof = &_sceneObjectFramesTable[so->frameNumPrev];
			uint32_t size = sof->hdr.w * sof->hdr.h + 4;
			bo->data = (uint8_t *)malloc(size);
			if (bo->data) {
				bo->dataSize = decodeLzss(sof->data, bo->data);
				assert(bo->dataSize == size);
			}

			++_bagObjectsCount;
			if (_bagObjectsCount != 0 && _currentBagObject == -1) {
				_currentBagObject = 0;
			}
		}
	}
}

void Game::oop_removeObjectFromBag() {
	debug(DBG_OPCODES, "Game::oop_removeObjectFromBag()");
	const char *name = _objectScript.fetchNextString();
	int index = findBagObjectByName(name);
	if (index != -1) {
		if (_currentBagObject == index) {
			if (_bagObjectsCount >= 1) {
				_currentBagObject = 0;
			} else {
				_currentBagObject = -1;
			}
		} else {
			if (_currentBagObject > index) {
				--_currentBagObject;
			}
		}
		free(_bagObjectsTable[index].data);
		int count = _bagObjectsCount - index - 1;
		if (count != 0) {
			memmove(&_bagObjectsTable[index], &_bagObjectsTable[index + 1], count * sizeof(BagObject));
		}
		--_bagObjectsCount;
	}
}

void Game::oop_playSoundLowerEqualPriority() {
	debug(DBG_OPCODES, "Game::oop_playSoundLowerEqualPriority()");
	int num = _objectScript.fetchNextWord();
	int priority = _objectScript.fetchNextWord();
	if (priority > _currentPlayingSoundPriority) {
		if (win16_sndPlaySound(22) == 0) {
			return;
		}
	}
	SceneObject *so = derefSceneObject(_objectScript.currentObjectNum);
	num += _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstSoundBufferIndex - 1;
	assert(num >= 0 && num < _soundBuffersCount);
	win16_sndPlaySound(3, _soundBuffersTable[num].filename); // win16_sndPlaySound(7, _soundBuffersTable[num].buffer);
	_currentPlayingSoundPriority = priority;
}

void Game::oop_playSoundLowerPriority() {
	debug(DBG_OPCODES, "Game::oop_playSoundLowerPriority()");
	int num = _objectScript.fetchNextWord();
	int priority = _objectScript.fetchNextWord();
	if (priority >= _currentPlayingSoundPriority) {
		if (win16_sndPlaySound(22) == 0) {
			return;
		}
	}
	SceneObject *so = derefSceneObject(_objectScript.currentObjectNum);
	num += _animationsTable[_sceneObjectMotionsTable[so->motionNum1].animNum].firstSoundBufferIndex - 1;
	assert(num >= 0 && num < _soundBuffersCount);
	win16_sndPlaySound(3, _soundBuffersTable[num].filename); // win16_sndPlaySound(7, _soundBuffersTable[num].buffer);
	_currentPlayingSoundPriority = priority;
}

void Game::oop_startDialogue() {
	debug(DBG_OPCODES, "Game::oop_startDialogue()");
	_scriptDialogId = _objectScript.fetchNextString();
	_scriptDialogFileName = _objectScript.fetchNextString();
	_scriptDialogSprite1 = _objectScript.fetchNextString();
	_scriptDialogSprite2 = _objectScript.fetchNextString();
	_startDialogue = true;
}

void Game::oop_switchSceneClearBoxes() {
	debug(DBG_OPCODES, "Game::oop_switchSceneClearBoxes()");
	bool foundScene = false;
	int num = _objectScript.fetchNextWord();
	for (int i = 0; i < _sceneConditionsCount; ++i) {
		if (_nextScenesTable[i].num == num) {
			_objectScript.nextScene = i;
			foundScene = true;
			break;
		}
	}
	if (foundScene) {
		for (int i = 0; i < 10; ++i) {
			_boxesCountTable[10 + i]  = 0;
		}
	}
}

void Game::oop_switchSceneCopyBoxes() {
	debug(DBG_OPCODES, "Game::oop_switchSceneCopyBoxes()");
	bool foundScene = false;
	int num = _objectScript.fetchNextWord();
	for (int i = 0; i < _sceneConditionsCount; ++i) {
		if (_nextScenesTable[i].num == num) {
			_objectScript.nextScene = i;
			foundScene = true;
			break;
		}
	}
	if (foundScene) {
		for (int i = 0; i < 10; ++i) {
			_boxesCountTable[10 + i] = _boxesCountTable[i];
			if (_boxesCountTable[i] != 0) {
				memcpy(&_boxesTable[10 + i][0], &_boxesTable[i][0], sizeof(Box) * _boxesCountTable[i]);
			}
		}
	}
}
