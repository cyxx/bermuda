
// g++ ripwinfont.cpp -lgdi32
#include <stdio.h>
#include <assert.h>
#include <windows.h>

static LOGFONT hSystemLogFont;
static HFONT hSystemFont;
static int gridW = 16;
static int gridH = 16;

static void loadSystemFont() {
	HGDIOBJ h = GetStockObject(SYSTEM_FONT);
	GetObject(h, sizeof(hSystemLogFont), &hSystemLogFont);
	hSystemFont = CreateFontIndirect(&hSystemLogFont);
	printf("hSystemFont %p w %d h %d\n", hSystemFont, hSystemLogFont.lfWidth, hSystemLogFont.lfHeight);
}

static void ripChar(int chr, int *dst, bool display) {
	HDC hDC = CreateCompatibleDC(NULL);
	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, gridW, gridH);
	HGDIOBJ hPrevBitmap = SelectObject(hDC, hBitmap);
	HGDIOBJ hPrevFont = SelectObject(hDC, hSystemFont);

	for (int yPos = 0; yPos < gridH; ++yPos) {
		for (int xPos = 0; xPos < gridW; ++xPos) {
			SetPixel(hDC, xPos, yPos, RGB(0, 0, 0));
		}
	}
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, RGB(255, 255, 255));
	char c = chr & 0xFF;
	TextOut(hDC, 0, 0, &c, 1);
	if (display) {
		for (int yPos = 0; yPos < gridH; ++yPos) {
			for (int xPos = 0; xPos < gridW; ++xPos) {
				COLORREF c = GetPixel(hDC, xPos, yPos);
				if (c == 0) {
					printf("0");
				} else {
					printf("1");
				}
			}
			printf("\n");
		}
	}
	for (int yPos = 0; yPos < gridH; ++yPos) {
		int bitMap = 0;
		for (int xPos = 0; xPos < gridW; ++xPos) {
			COLORREF c = GetPixel(hDC, xPos, yPos);
			if (c != 0) {
				bitMap |= 1 << xPos;
			}
		}
		dst[yPos] = bitMap;
	}

	SelectObject(hDC, hPrevFont);
	SelectObject(hDC, hPrevBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hDC);
}

static void getCharPitch(int *src, int *p) {
	int maxWidth = 0;
	for (int yPos = 0; yPos < gridH; ++yPos) {
		int w = 0;
		for (int xPos = 0; xPos < gridW; ++xPos) {
			if (src[yPos] & (1 << xPos)) {
				w = xPos;
			}
		}
		if (w != 0) {
			++w;
			if (w > maxWidth) {
				maxWidth = w;
			}
		}
	}
	*p = maxWidth;
}

static void dumpFont(int *src, int *src2) {
	printf("const uint16 Game::_fontData[] = {");
	for (int i = 0; i < gridH * 256; ++i) {
		if ((i % 10) == 0) {
			printf("\n\t");
		}
		printf("0x%04X, ", src[i]);
	}
	printf("\n};\n");
	printf("const uint8 Game::_fontCharWidth[] = {");
	for (int i = 0; i < 256; ++i) {
		if ((i % 16) == 0) {
			printf("\n\t");
		}
		printf("0x%02X, ", src2[i]);
	}
	printf("\n};\n");
}

int main(int argc, char *argv[]) {
	printf("sizeof(HFONT) %d sizeof(LOGFONT) %d\n", sizeof(HFONT), sizeof(LOGFONT));
	loadSystemFont();
	int *fontData = (int *)malloc(sizeof(int) * gridH * 256);
	for (int i = 0; i < 256; ++i) {
		ripChar(i, &fontData[gridH * i], false);
	}
	int *fontCharWidth = (int *)malloc(sizeof(int) * 256);
	for (int i = 0; i < 256; ++i) {
		getCharPitch(&fontData[gridH * i], &fontCharWidth[i]);
	}
	fontCharWidth[0x20] = 4;
	dumpFont(fontData, fontCharWidth);
//	ripChar('m', &fontData[0], true);
//	printf("w %d\n", fontCharWidth['m']);
	return 0;
}
