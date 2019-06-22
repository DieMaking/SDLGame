NAME = SDLGame
SRC = ./src
TMP = ./temp
WIN_BD = $(NAME)_Windows
WIN_TMP = temp
CR = g++
CRFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-write-strings -std=c++11
LRFLAGS = 
LRLIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -ldiscord-rpc -pthread
ATTR = --debug

ifeq ($(BUILD), release)
	# Release build - optimization and no debugging symbols
	CRFLAGS += -O2 -s -DNDEBUG
	LRFLAGS += -O2 -s
else
	# Debug build - no optimization and debugging symbols
	CRFLAGS += -O0 -g
	LRFLAGS += -O0 -g
endif

ifeq ($(MSYSTEM), MINGW32)
	BD = $(NAME)_Windows
	DEL = rm -rf "$(BD)/$(NAME)" "$(TMP)/*"
	TIME = `date +%T`
	NL = echo ""
	TEST = cd "$(WIN_BD)" && ./$(NAME) $(ATTR)
	RES = windres "$(SRC)/resources.rc" -o "$(TMP)/resources.o"
	RES2 = "$(TMP)/resources.o"
	LRFLAGS += -mwindows
	LRLIBS += -lWs2_32
else ifeq ($(OS), Windows_NT)
	BD = $(NAME)_Windows
	DEL = del /f /q "$(WIN_BD)/$(NAME).exe" "$(WIN_TMP)/*" >nul 2>nul
	TIME = %date% %time:~0,8%
	NL = echo.
	TEST = cd "$(WIN_BD)" & start "$(NAME)" cmd /c "$(NAME).exe $(ATTR) & echo. & pause"
	RES = windres "$(SRC)/resources.rc" -o "$(TMP)/resources.o"
	RES2 = "$(TMP)/resources.o"
	CRFLAGS += -IC:/MinGW/include
	LRFLAGS += -LC:/MinGW/lib
	LRFLAGS += -mwindows
	LRLIBS += -lWs2_32
else
	BD = $(NAME)_Linux
	DEL = rm -rf "$(BD)/$(NAME)" "$(TMP)/*"
	TIME = `date +%T`
	NL = echo ""
	TEST = cd "$(BD)" && cp -f "$(NAME)" "/tmp/$(NAME)" && chmod +x "/tmp/$(NAME)" && xfce4-terminal -T "$(NAME)" -e "/tmp/$(NAME) $(ATTR)" && rm -f "/tmp/$(NAME)"
	RES = 
	RES2 = 
endif

all: info clean compile

compile: resources main renderer
	$(CR) $(LRFLAGS) $(RES2) "$(TMP)/main.o" "$(TMP)/renderer.o" $(LRLIBS) -o "$(BD)/$(NAME)"

clean:
	-@$(DEL)

info:
	@echo =====================================
	@echo   Compile Time: $(TIME)
	@echo =====================================
	@$(NL)

run test:
	@$(TEST)

resources:
	$(RES)

main:
	$(CR) $(CRFLAGS) "$(SRC)/main.cpp" -c -o "$(TMP)/main.o"

renderer:
	$(CR) $(CRFLAGS) "$(SRC)/renderer.cpp" -c -o "$(TMP)/renderer.o"
