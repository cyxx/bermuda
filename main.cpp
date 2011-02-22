/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include "game.h"
#include "systemstub.h"

static SystemStub *g_stub;

static const char *USAGE =
	"Bermuda Syndrome\n"
	"Usage: bs [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default 'DATA')\n"
	"  --savepath=PATH   Path to save files (default '.')\n"
	"  --musicpath=PATH  Path to music files (default 'MUSIC')\n";

static bool parseOption(const char *arg, const char *longCmd, const char **opt) {
	bool handled = false;
	if (arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg + 2, longCmd, strlen(longCmd)) == 0) {
			*opt = arg + 2 + strlen(longCmd);
			handled = true;
		}
	}
	return handled;
}

static void exitMain() {
	if (g_stub) {
		g_stub->destroy();
		delete g_stub;
		g_stub = 0;
	}
}

#undef main
int main(int argc, char *argv[]) {
	const char *dataPath = "DATA";
	const char *savePath = ".";
	const char *musicPath = "MUSIC";
	for (int i = 1; i < argc; ++i) {
		bool opt = false;
		if (strlen(argv[i]) >= 2) {
			opt |= parseOption(argv[i], "datapath=", &dataPath);
			opt |= parseOption(argv[i], "savepath=", &savePath);
			opt |= parseOption(argv[i], "musicpath=", &musicPath);
		}
		if (!opt) {
			printf("%s", USAGE);
			return 0;
		}
	}
	g_debugMask = DBG_INFO; // | DBG_GAME | DBG_OPCODES | DBG_DIALOGUE;
	g_stub = SystemStub_SDL_create();
	atexit(exitMain);
	Game *g = new Game(g_stub, dataPath, savePath, musicPath);
	g->mainLoop();
	delete g;
	return 0;
}
