
# List of possible defines :
# BERMUDA_WIN32  : enable windows directory browsing code
# BERMUDA_POSIX  : enable unix/posix directory browsing code
# BERMUDA_VORBIS : enable playback of digital soundtracks (22 khz mono .ogg files)
# BERMUDA_BLUR   : enable blur rendering effect (use 'b' key to toggle effect on/off)

#DEFINES = -DBERMUDA_WIN32 -DBERMUDA_VORBIS
DEFINES = -DBERMUDA_POSIX -DBERMUDA_VORBIS
VORBIS_LIBS = -lvorbisfile -lvorbis -logg

SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

CXX = g++
CXXFLAGS = -g -O -Wall $(SDL_CFLAGS) $(DEFINES)

-include Makefile.local

OBJDIR = obj

SRCS = avi_player.cpp bag.cpp decoder.cpp dialogue.cpp file.cpp fs.cpp game.cpp \
	main.cpp mixer.cpp opcodes.cpp parser_dlg.cpp parser_scn.cpp random.cpp \
	resource.cpp saveload.cpp staticres.cpp str.cpp systemstub_sdl.cpp util.cpp \
	win16.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all: $(OBJDIR) bs

bs: $(addprefix $(OBJDIR)/, $(OBJS))
	$(CXX) $(LDFLAGS) -o $@ $^ $(SDL_LIBS) $(VORBIS_LIBS)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d

-include $(addprefix $(OBJDIR)/, $(DEPS))
