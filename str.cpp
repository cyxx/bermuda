/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "str.h"

void stringToLowerCase(char *p) {
	for (; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

void stringToUpperCase(char *p) {
	for (; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}

static bool isIgnorableChar(char c) {
	return (c == 0x60); // C1_05.SCN
}

static bool isWhitespace(char c) {
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || isIgnorableChar(c));
}

char *stringTrimRight(char *p) {
	char *s = p;
	int len = strlen(p);
	if (len != 0) {
		p += len - 1;
		while (p >= s && isWhitespace(*p)) {
			*p = 0;
			--p;
		}
	}
	return s;
}

char *stringTrimLeft(char *p) {
	while (*p && isWhitespace(*p)) {
		*p = 0;
		++p;
	}
	return p;
}

char *stringNextToken(char **p) {
	char *token = stringTrimLeft(*p);
	char *end = token;
	if (token) {
		if (*token == '"') {
			++token;
			while (*end && *end != '"') {
				++end;
			}
		} else {
			while (*end && !isWhitespace(*end)) {
				++end;
			}
		}
		if (*end) {
			*end = 0;
			*p = end + 1;
		} else {
			*p = 0;
		}
	}
	return token;
}

char *stringNextTokenEOL(char **p) {
	char *token = stringTrimLeft(*p);
	if (token) {
		char *buf = strstr(token, "\r\n");
		if (buf) {
			*buf = 0;
			*p = &buf[2];
		} else {
			*p = 0;
		}
	}
	return token;
}

void stringStripComments(char *p) {
	for (char *str = p; *str; ++str) {
		if (*str == '\t') {
			*str = ' ';
		}
	}
	while (true) {
		char *cmt = strstr(p, "//");
		if (!cmt) {
			break;
		}
		const char *eol = strstr(cmt, "\r\n");
		while (*cmt && cmt != eol) {
			*cmt++ = ' ';
		}
	}
	char *cmt = p;
	while (true) {
		while (*cmt && isWhitespace(*cmt)) {
			++cmt;
		}
		if (!*cmt) {
			break;
		}
		char *eol = strstr(cmt, "\r\n");
		if (cmt[0] == ';' || cmt[0] == '/') {
			while (*cmt && cmt != eol) {
				*cmt++ = ' ';
			}
		}
		if (!eol) {
			break;
		}
		cmt = &eol[2];
	}
}

bool stringEndsWith(const char *p, const char *suf) {
	const int offs = strlen(p) - strlen(suf);
	return (offs >= 0) && (strcmp(p + offs, suf) == 0);
}
