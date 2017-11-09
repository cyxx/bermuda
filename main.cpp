/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#include <getopt.h>
#include <sys/stat.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "game.h"
#include "systemstub.h"

static const char *USAGE =
	"Bermuda Syndrome\n"
	"Usage: bs [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default 'DATA')\n"
	"  --savepath=PATH   Path to save files (default '.')\n"
	"  --musicpath=PATH  Path to music files (default 'MUSIC')\n";

static Game *g_game;
static SystemStub *g_stub;

static void init(const char *dataPath, const char *savePath, const char *musicPath) {
	g_stub = SystemStub_SDL_create();
	g_game = new Game(g_stub, dataPath ? dataPath : "DATA", savePath ? savePath : ".", musicPath ? musicPath : "MUSIC");
	g_game->init();
}

static void fini() {
	g_game->fini();
	delete g_game;
	delete g_stub;
	g_stub = 0;
}

#ifdef __EMSCRIPTEN__
static void mainLoop() {
	if (!g_stub->_quit) {
		g_game->mainLoop();
	}
}
#endif

#undef main
int main(int argc, char *argv[]) {
	char *dataPath = 0;
	char *savePath = 0;
	char *musicPath = 0;
	if (argc == 2) {
		// data path as the only command line argument
		struct stat st;
		if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
			dataPath = strdup(argv[1]);
		}
	}
	while (1) {
		static struct option options[] = {
			{ "datapath",  required_argument, 0, 'd' },
			{ "savepath",  required_argument, 0, 's' },
			{ "musicpath", required_argument, 0, 'm' },
			{ "help",      no_argument,       0, 'h' },
			{ 0, 0, 0, 0 }
		};
		int index;
		const int c = getopt_long(argc, argv, "", options, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'd':
			dataPath = strdup(optarg);
			break;
		case 's':
			savePath = strdup(optarg);
			break;
		case 'm':
			musicPath = strdup(optarg);
			break;
		default:
			fprintf(stdout, "%s", USAGE);
			return 0;
		}
	}
	g_debugMask = DBG_INFO; // | DBG_GAME | DBG_OPCODES | DBG_DIALOGUE;
	init(dataPath, savePath, musicPath);
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(mainLoop, kCycleDelay, 0);
#else
	uint32_t lastFrameTimeStamp = g_stub->getTimeStamp();
	while (!g_stub->_quit) {
		g_game->mainLoop();
		const uint32_t end = lastFrameTimeStamp + kCycleDelay;
		do {
			g_stub->sleep(10);
			g_stub->processEvents();
		} while (!g_stub->_pi.fastMode && g_stub->getTimeStamp() < end);
		lastFrameTimeStamp = g_stub->getTimeStamp();
	}
	fini();
#endif
	free(dataPath);
	free(savePath);
	free(musicPath);
	return 0;
}
