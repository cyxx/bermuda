/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "game.h"
#include "systemstub.h"

void Game::handleBagMenu() {

	const int xPosWnd = (kGameScreenWidth  - (_bagBackgroundImage.w + 1)) / 2;
	const int yPosWnd = (kGameScreenHeight - (_bagBackgroundImage.h + 1)) / 4;

		if (_stub->_pi.leftMouseButton) {
			_stub->_pi.leftMouseButton = false;
			int xPos = _stub->_pi.mouseX - xPosWnd;
			int yPos = _stub->_pi.mouseY - yPosWnd;

			// action area
			for (int i = 0; i < 3; ++i) {
				if (xPos >= i * 45 + 118 && xPos < i * 45 + 160) {
					if (yPos >= 15 && yPos < 55) {
						if (i != _currentBagAction) {
							_currentBagAction = i;
						}
					}
				}
			}
			// bag object area
			for (int i = 0; i < MIN(_bagObjectsCount, 4); ++i) {
				if (xPos >= i * 32 + 261 && xPos < i * 32 + 293) {
					if (yPos >= 4 && yPos < 44) {
						if (i != _currentBagObject || _currentBagAction != 3) {
							_currentBagObject = i;
							_currentBagAction = 3; // kActionUseObject
						}
					}
				}
			}
			// weapons area
			if (xPos >= 22 && xPos < getBitmapWidth(_weaponIconImageTable[0]) + 22) {
				if (yPos >= 22 && yPos < getBitmapHeight(_weaponIconImageTable[0]) + 22) {
					if (_varsTable[1] == 1 && _varsTable[2] != 0) { // switch to gun
						_varsTable[2] = 1;
						_varsTable[1] = 2;
					}
				}
			}
			if (xPos >= 22 && xPos < getBitmapWidth(_swordIconImage) + 22) {
				if (yPos >= 37 && yPos < getBitmapHeight(_swordIconImage) + 37) {
					if (_varsTable[2] == 1 && _varsTable[1] != 0) { // switch to ?
						_varsTable[2] = 2;
						_varsTable[1] = 1;
					}
				}
			}
		}

		if (_stub->_pi.tab) {
			_stub->_pi.tab = false;
			_nextState = kStateGame;
			return;
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			_nextState = kStateGame;
			return;
		}

		if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			if (_currentBagAction != 0) {
				if (_currentBagAction != 3 || _currentBagObject <= 0) {
					--_currentBagAction;
				} else {
					--_currentBagObject;
				}
			}
		}

		if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			if (_currentBagAction == 3) {
				if (_bagObjectsCount - 1 > _currentBagObject && _currentBagObject != -1) {
					++_currentBagObject;
				}
			} else {
				++_currentBagAction;
			}
		}

		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (_varsTable[2] == 1 && _varsTable[1] != 0) {
				_varsTable[2] = 2;
				_varsTable[1] = 1;
			}
		}

		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (_varsTable[1] == 1 && _varsTable[2] != 0) {
				_varsTable[2] = 1;
				_varsTable[1] = 2;
			}
		}

		drawBagMenu(xPosWnd, yPosWnd);

		++_lifeBarCurrentFrame;
		if (_lifeBarCurrentFrame >= 12) {
			_lifeBarCurrentFrame = 0;
		}
		if (_currentBagObject >= 0 && _currentBagObject < 4 && _currentBagAction == 3 && _bagObjectsCount != 0) {
			++_bagObjectAreaBlinkCounter;
			if (_bagObjectAreaBlinkCounter >= 10) {
				_bagObjectAreaBlinkCounter = 0;
			}
		}
		if (_varsTable[1] == 1) {
			++_bagWeaponAreaBlinkCounter;
			if (_bagWeaponAreaBlinkCounter >= 10) {
				_bagWeaponAreaBlinkCounter = 0;
			}
		} else if (_varsTable[2] == 1) {
			++_bagWeaponAreaBlinkCounter;
			if (_bagWeaponAreaBlinkCounter >= 10) {
				_bagWeaponAreaBlinkCounter = 0;
			}
		}
}

void Game::drawBagMenu(int xPosWnd, int yPosWnd) {
	static const struct {
		int x, y;
	} _bagPosItems[] = {
		{ 121, 17 },
		{ 164, 26 },
		{ 222, 16 }
	};
	for (int i = 0; i < 4; ++i) {
		int y = _bagBackgroundImage.h - 32 - getBitmapHeight(_bagObjectAreaBlinkImageTable[0]);
		drawObject((_isDemo ? 247 : 269) + i * 32, y, _bagObjectAreaBlinkImageTable[0], &_bagBackgroundImage);
	}

	// make a copy for drawing (will be restored at the end)
	SceneBitmap backup = _bagBackgroundImage;
	int size = (backup.w + 1) * (backup.h + 1);
	backup.bits = (uint8_t *)malloc(size);
	if (!backup.bits) {
		return;
	}
	memcpy(backup.bits, _bagBackgroundImage.bits, size);

	if (_varsTable[2] != 0) {
		int index = MIN(13 - _varsTable[4], 13);
		uint8_t *p = _weaponIconImageTable[index];
		int y = _bagBackgroundImage.h - (_isDemo ? 19 : 21) - getBitmapHeight(p);
		drawObject(22, y, p, &_bagBackgroundImage);
		if (_varsTable[3] < 5) {
			index = _varsTable[4] <= 0 ? 0 : 1;
			p = _ammoIconImageTable[index][_varsTable[3]];
			y = _bagBackgroundImage.h - (_isDemo ? 29 : 31) - getBitmapHeight(p);
			drawObject(33, y, p, &_bagBackgroundImage);
		}
	}
	if (!_isDemo && _varsTable[1] != 0) {
		int y = _bagBackgroundImage.h - 36 - getBitmapHeight(_swordIconImage);
		drawObject(22, y, _swordIconImage, &_bagBackgroundImage);
	}
	int index = (_varsTable[0] >= 10) ? 10 : _varsTable[0];
	uint8_t *lifeBarFrame = _lifeBarImageTable[index][_lifeBarCurrentFrame];
	drawObject(_isDemo ? 23 : 314, _bagBackgroundImage.h - (_isDemo ? 52 : 51) - getBitmapHeight(lifeBarFrame), lifeBarFrame, &_bagBackgroundImage);
	if (_currentBagAction < 3) {
		uint8_t *p = _bermudaSprDataTable[_currentBagAction];
		int y = _bagBackgroundImage.h - _bagPosItems[_currentBagAction].y - getBitmapHeight(p);
		int x = _bagPosItems[_currentBagAction].x;
		if (_isDemo) {
			x -= 22;
		}
		drawObject(x, y, p, &_bagBackgroundImage);
	}
	for (int i = 0; i < MIN<int>(_bagObjectsCount, 4); ++i) {
		uint8_t *bagObjectData = _bagObjectsTable[i].data;
		int x = (_isDemo ? 239 : 261) + i * 32 + (32 - getBitmapWidth(bagObjectData)) / 2;
		int y = _bagBackgroundImage.h - 3 - (40 - getBitmapHeight(bagObjectData)) / 2 - getBitmapHeight(bagObjectData);
		drawObject(x, y, bagObjectData, &_bagBackgroundImage);
	}
	if (_currentBagObject >= 0 && _currentBagObject < 4 && _bagObjectsCount != 0) {
		uint8_t *p = _currentBagAction == 3 ? _bagObjectAreaBlinkImageTable[_bagObjectAreaBlinkCounter] : _bagObjectAreaBlinkImageTable[0];
		drawObject((_isDemo ? 247 : 269) + _currentBagObject * 32, _bagBackgroundImage.h - 32 - getBitmapHeight(p), p, &_bagBackgroundImage);
	}
	if (!_isDemo) {
		uint8_t *weaponImage1 = _varsTable[1] == 1 ? _bagWeaponAreaBlinkImageTable[_bagWeaponAreaBlinkCounter] : _bagWeaponAreaBlinkImageTable[0];
		drawObject(87, _bagBackgroundImage.h - 38 - getBitmapHeight(weaponImage1), weaponImage1, &_bagBackgroundImage);
		uint8_t *weaponImage2 = _varsTable[2] == 1 ? _bagWeaponAreaBlinkImageTable[_bagWeaponAreaBlinkCounter] : _bagWeaponAreaBlinkImageTable[0];
		drawObject(87, _bagBackgroundImage.h - 25 - getBitmapHeight(weaponImage2), weaponImage2, &_bagBackgroundImage);
	}

	_stub->copyRect(xPosWnd, yPosWnd, _bagBackgroundImage.w + 1, _bagBackgroundImage.h + 1, _bagBackgroundImage.bits, _bagBackgroundImage.pitch);

	memcpy(_bagBackgroundImage.bits, backup.bits, size);
	free(backup.bits);
}
