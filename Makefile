CXX      = g++
WINDRES  = windres
SDL2_INC := $(shell cygpath -m /mingw64/include/SDL2)
SDL2_LIB := $(shell cygpath -m /mingw64/lib)

CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude -I$(SDL2_INC)
LDFLAGS  = -L$(SDL2_LIB)
LIBS     = -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lgdi32 -lcomdlg32

SRC = src/main.cpp \
      src/midi_reader.cpp \
      src/apu.cpp \
      src/dmc.cpp \
      src/i18n.cpp \
      src/preset.cpp \
      src/wav_writer.cpp \
      src/builtin_presets.cpp \
      src/gui/imgui.cpp \
      src/gui/imgui_draw.cpp \
      src/gui/imgui_tables.cpp \
      src/gui/imgui_widgets.cpp \
      src/gui/imgui_demo.cpp \
      src/gui/imgui_impl_sdl2.cpp \
      src/gui/imgui_impl_opengl3.cpp

OBJ = $(SRC:.cpp=.o)
RES = app.res.o
BIN = nes_midi_player_v1.0.exe

all: $(BIN)

$(BIN): $(OBJ) $(RES)
	$(CXX) $(OBJ) $(RES) -o $@ $(LDFLAGS) $(LIBS)

app.res.o: app.rc icon.ico
	$(WINDRES) app.rc -O coff -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN) $(RES)

.PHONY: all clean
