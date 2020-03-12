#ifndef __GAME_HPP
#define __GAME_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <ctime>
#include <map>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#define SYSTEM_WINDOWS
	#include <windows.h>
	#include <fcntl.h>
#endif

// Starting frame and key used for enabling/disabling counter
#ifdef __EMSCRIPTEN__
	#define DEFAULT_FRAME 2
	#define COUNTER_KEYCODE SDL_SCANCODE_GRAVE
#else
	#define DEFAULT_FRAME 4
	#define COUNTER_KEYCODE SDL_SCANCODE_F1
#endif

// Game title and version
const char* title = "SDLGame";
const char* version = "v0.0.9 Pre-release";

// Window dimensions
int width = 800;
int height = 600;

// Player dimensions
int sizeX = 38;
int sizeY = 48;

// Player position
double posX = 30;
double posY = height - sizeY;
double tmpX, tmpY;

// Static FPS value
uint32_t fps = 60;

// For resizing and setting position
SDL_Rect rect;

// For flipping character
SDL_RendererFlip flip;

// For events
SDL_Event e;

// For keyboard handling
const uint8_t* key;

// For mouse handling
int mouseX, mouseY;
int lastX, lastY;
uint32_t mouse;

// For FPS counting
uint32_t fpsFrameTicks;
uint32_t fpsFrames;
uint32_t fpsCount = 0;

// Main menu option
uint8_t option = 1;

// Key locks
bool escKey;
bool optKey;
bool enterKey;
bool jumpKey;
bool counterKey;
bool mouseLock;

// Switches
bool debug;
bool demo;
bool isPlaying;
bool showCounter;

// Resources
SDL_Texture* bg;
SDL_Texture* menubg;
SDL_Texture* player;
SDL_Texture* text;
SDL_Texture* overlay;
SDL_Texture* trackbar;
SDL_Texture* counter0;
SDL_Texture* counter1;
SDL_Texture* counter2;
SDL_Texture* counter3;
SDL_Texture* counter4;
SDL_Texture* counter5;

// Option resources
SDL_Texture* volumeLabel;

// Colors
SDL_Color black = { 0, 0, 0, 255 };
SDL_Color red = { 255, 0, 0, 255 };
SDL_Color green = { 0, 210, 60, 255 };
SDL_Color white = { 255, 255, 255, 255 };
SDL_Color dimwhite = { 230, 230, 230, 255 };

// Fonts
TTF_Font* buttonFont;
TTF_Font* optionFont;
TTF_Font* counterFont;

// Sounds
Mix_Chunk* bgsound;
uint8_t volume = 128;

// Frame values
uint32_t frame = DEFAULT_FRAME;
uint32_t lastFrame;
uint32_t lastGameFrame;
uint32_t gameFrame = 1;
int8_t gameFrameChange;
#define GAME_FRAMES 3

// Render targets (for the game frame scrolling)
SDL_Texture* render1;
SDL_Texture* render2;
int renderPos;

// Gravity values
uint8_t jumpState;
double gravity = 600.0;
double velocityY;
double speed = 200.0;
double jumpStrength = 350.0;
double delta = 0.02;

// Demo walking direction
bool demoDirection;

// Collisions (TODO)
uint8_t collisionCounts[GAME_FRAMES] = { 2, 1, 1 };
SDL_Rect* collisions[GAME_FRAMES] = {
	new SDL_Rect[2] {
		{ 460, 440, 50, 15 },
		{ 630, 540, 60, 15 }
	}, new SDL_Rect[1] {
		{ 620, 540, 70, 15 }
	}, new SDL_Rect[1] {
		{ 610, 540, 80, 15 }
	}
};

// Dialog box data
struct {
	std::string text = "";
	std::string buttonText = "OK";
	void Set(std::string text = "", std::string buttonText = "OK") {
		this->text = text;
		this->buttonText = buttonText;
	}
} dialogBox;

#ifndef __EMSCRIPTEN__
	// Used to count delay between frames
	uint32_t frameTicks;

	// Used to end main loop
	bool quit;

	// Server connection
	easysock::tcp::Client* conn;
	bool connected;
	bool skipconnect;

	// Is user a spectator?
	bool spectating;

	// Startup thread
	pthread_t thread;
	void* StartupThread(void*);
#endif

bool ReopenConsole() {
	#ifdef SYSTEM_WINDOWS
		// If console window is not opened
		if(!GetConsoleWindow()) {
			// Open new console window
			if(!AllocConsole()) {
				return false;
			}
		}

		// Set console title
		SetConsoleTitle(title);

		// Allocate stdout to opened console
		// freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		int hCrt = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
		FILE *hf = _fdopen(hCrt, "w");
		*stdout = *hf;
		setvbuf(stdout, NULL, _IONBF, 0);

		// Allocate stderr to opened console
		// freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
		hCrt = _open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
		hf = _fdopen(hCrt, "w");
		*stderr = *hf;
		setvbuf(stderr, NULL, _IONBF, 0);
	#endif
	return true;
}

void DisplayDialog(std::string msg, bool error = false) {
	#if defined(__EMSCRIPTEN__)
		EM_ASM({
			if(alert) alert($0);
		}, msg.c_str());
	#elif defined(SYSTEM_WINDOWS)
		MessageBox(NULL, msg.c_str(), title, error ? MB_ICONERROR : 0);
	#else
		SDL_ShowSimpleMessageBox(error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION, title, msg.c_str(), NULL);
	#endif
}

void DisplayInfo(std::string msg) {
	std::cout << msg << std::endl;
	DisplayDialog(msg);
}

void DisplayError(std::string err) {
	std::cerr << err << std::endl;
	DisplayDialog(err, true);
}

void Log(std::string msg) {
	if(debug) {
		time_t tval;
		time(&tval); 
		tm* tobj = localtime(&tval); 
		char str[12];
		strftime(str, 12, "[%X] ", tobj);
		std::cout << str << msg << std::endl;
	}
}

/*bool CheckCollision() {
	for(uint8_t i = 0; i < collisionCounts[gameFrame - 1]; i++) {
		rect = collisions[gameFrame - 1][i];
		if(posX + sizeX > rect.x && posX < rect.x + rect.w && posY + sizeY > rect.y && posY < rect.y + rect.h) {
			return true;
		}
	}
	return false;
}*/

char* RandomStr(char* target, int len, const char* chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
	if(len > 100) return NULL;
	int charsLen = strlen(chars) - 1;
	char buf[100];
	for(int i = 0; i < len; i++) {
		buf[i] = chars[rand() % charsLen];
	}
	return strcpy(target, buf);
}

std::string NumToStr(double num, int places = 2) {
	char fmt[15];
	if(places > 0) {
		if(snprintf(fmt, 15, "%%.%dlf", places) < 0) return "";
	} else {
		strcpy(fmt, "%.0lf");
	}
	std::string str(100, '\0');
	str.resize(snprintf(&str[0], 100, fmt, num));
	return str;
}

std::vector<std::string> Split(std::string input, std::string delim, int c = 0) {
	std::string temp;
	std::vector<std::string> out;
	int i = 0;
	size_t oldpos = 0;
	size_t pos = 0;
	while(true) {
		if(c < 1 || i < c) {
			pos = input.find(delim, oldpos);
			if(pos == std::string::npos) break;
			temp = input.substr(oldpos, pos - oldpos);
			out.push_back(temp);
		} else {
			temp = input.substr(oldpos);
			out.push_back(temp);
			break;
		}
		oldpos = pos + delim.length();
		i++;
	}
	return out;
}

std::string Join(std::vector<std::string> input, std::string delim) {
	std::string data = "";
	for(auto const &value: input) {
		data += value + delim;
	}
	return data;
}

std::string Serialize(uint32_t frame, double x, double y) {
	return std::to_string(frame) + " " + std::to_string(x) + " " + std::to_string(y);
}

bool Unserialize(std::string data, uint32_t &frame, double &x, double &y) {
	auto vars = Split(data, " ");
	if(vars.size() != 3) {
		return false;
	}
	frame = std::stoi(vars[0]);
	x = std::stoi(vars[1]);
	y = std::stoi(vars[2]);
	return true;
}

#endif
