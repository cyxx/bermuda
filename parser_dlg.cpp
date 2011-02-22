/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "fs.h"
#include "file.h"
#include "game.h"
#include "str.h"

enum ParserToken {
	kParserTokenGoto,
	kParserTokenEnd,
	kParserTokenThen,
	kParserTokenEOS,
	kParserTokenUnknown
};

static ParserToken _currentToken;

static ParserToken getNextToken(char **s) {
	char *token = stringNextToken(s);
	if (!token || !*token) {
		return kParserTokenEOS;
	} else if (strcmp(token, "Goto") == 0) {
		return kParserTokenGoto;
	} else if (strcmp(token, "End") == 0) {
		return kParserTokenEnd;
	} else if (strcmp(token, "->") == 0) {
		return kParserTokenThen;
	} else {
		return kParserTokenUnknown;
	}
}

void Game::parseDLG() {
	_dialogueChoiceSize = 0;
	for (char *s = _dialogueDescriptionBuffer; s; ) {
		assert(_dialogueChoiceSize < NUM_DIALOG_ENTRIES);
		DialogueChoice *dc = &_dialogueChoiceData[_dialogueChoiceSize];
		dc->id = stringNextToken(&s);
		if (!s) {
			break;
		}
		_currentToken = getNextToken(&s);
		switch (_currentToken) {
		case kParserTokenGoto:
			dc->gotoFlag = true;
			break;
		case kParserTokenEnd:
			dc->gotoFlag = false;
			break;
		default:
			error("Unexpected token %d", _currentToken);
			break;
		}
		dc->nextId = stringNextToken(&s);
		dc->speechSoundFile = stringNextToken(&s);
		_currentToken = getNextToken(&s);
		if (_currentToken != kParserTokenThen) {
			error("Unexpected token %d", _currentToken);
		}
		dc->text = stringNextTokenEOL(&s);
		++_dialogueChoiceSize;
	}
}
