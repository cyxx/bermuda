
#include "game.h"
#include "systemstub.h"

void Game::initMenu() {
	_menuOption = -1;
	_menuObjectCount = _sceneObjectsCount;
	_menuObjectMotion = _sceneObjectMotionsCount;
	_menuObjectFrames = _sceneObjectFramesCount;
	loadWGP("..\\menu\\menu2.wgp");
	loadMOV("..\\menu\\menu2.mov");
	assert(_menuObjectMotion + 1 == _sceneObjectMotionsCount);
	_stub->setPalette(_bitmapBuffer0 + kOffsetBitmapPalette, 256);
	_stub->showCursor(true);
}

void Game::finiMenu() {
	_sceneObjectsCount = _menuObjectCount;
	_sceneObjectMotionsCount = _menuObjectMotion;
	_sceneObjectFramesCount = _menuObjectFrames;
	const int state = _loadDataState;
	loadWGP(_currentSceneWgp);
	_loadDataState = state;
	_stub->setPalette(_bitmapBuffer0 + kOffsetBitmapPalette, 256);
	_stub->showCursor(false);
}

void Game::handleMenu() {
	const int xCursor = _stub->_pi.mouseX;
	const int yCursor = _stub->_pi.mouseY;
	for (int frame = 0; frame < _sceneObjectMotionsTable[_menuObjectMotion].count; ++frame) {
		SceneObjectFrame *sof = &_sceneObjectFramesTable[_sceneObjectMotionsTable[_menuObjectMotion].firstFrameIndex + frame];
		const int yPos = _bitmapBuffer1.h + 1 - sof->hdr.yPos - sof->hdr.h;
		if (xCursor >= sof->hdr.xPos && xCursor < sof->hdr.xPos + sof->hdr.w && yCursor >= sof->hdr.yPos && yCursor < sof->hdr.yPos + sof->hdr.h) {
			sof->decode(sof->data, _tempDecodeBuffer);
			drawObject(sof->hdr.xPos, yPos, _tempDecodeBuffer, &_bitmapBuffer1);
			if (_stub->_pi.leftMouseButton) {
				_stub->_pi.leftMouseButton = false;
				_menuOption = frame;
			}
		} else {
			copyBufferToBuffer(sof->hdr.xPos, yPos, sof->hdr.w, sof->hdr.h, &_bitmapBuffer3, &_bitmapBuffer1);
		}
	}
	_stub->copyRect(0, 0, kGameScreenWidth, kGameScreenHeight, _bitmapBuffer1.bits, _bitmapBuffer1.pitch);
}
