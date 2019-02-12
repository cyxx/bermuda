
Bermuda Syndrome README
Release version: 0.1.7
-------------------------------------------------------------------------------


About:
------

This program is a re-implementation of the engine used in the game Bermuda
Syndrome made by Century Interactive and published by BMG in 1995. The original
data files are required (retail or demo versions).

This rewrite is based on the reversing of the original Windows executable. More
informations about the game can be found at [1] and [2].


Running:
--------

By default, the engine will try to load the game data files from the 'DATA'
directory. The savestates are saved in the current directory. If you have the
CD game soundtrack, you can rip the tracks to 22 Khz stereo Vorbis .ogg files.

The paths can be changed using command line switches :

	Usage: bs [OPTIONS]...
  	--datapath=PATH    Path to data files (default 'DATA')
  	--savepath=PATH    Path to save files (default '.')
	--musicpath=MUSIC  Path to music files (default 'MUSIC')
	--fullscreen       Fullscreen display
	--widescreen=MODE  Widescreen mode ('default', '4:3' or '16:9')

Game hotkeys :

	Arrow Keys      move Jack
	Tab             display bag
	Ctrl            display status bar
	Shift/Space     use weapon
	Enter           skip dialogue speech

Other hotkeys:

	C		capture screenshot as .tga
	S               save game state
	L               load game state
	+               increase game state slot
	-               decrease game state slot
	F               enable fast mode
	W               toggle fullscreen/windowed display


Contact:
--------

Gregory Montoir, cyx@users.sourceforge.net


URLs:
-----

[1] http://www.mobygames.com/game/bermuda-syndrome
[2] http://www.reakktor.com/about
