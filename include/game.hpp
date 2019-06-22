#ifndef __GAME_HPP
#define __GAME_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <map>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#define SYSTEM_WINDOWS
	#include <windows.h>
	#include <fcntl.h>
#endif

// Window title
const char* title = "SDLGame";

// Version string
const char* version = "v0.0.7 Pre-release";

// Window dimensions
int width = 800;
int height = 600;

// Player dimensions
int sizeX = 38;
int sizeY = 48;

// Default player position
double posX = 30;
double posY = height - sizeY;

// Game frames count
int gameFrames = 3;

// Static FPS value
uint32_t fps = 60;

// Debugging
bool debug = false;

// Demo
bool demo = false;

// Starting frame and key used for enabling/disabling counter
#ifdef __EMSCRIPTEN__
	#define DEFAULT_FRAME 2
	#define COUNTER_KEYCODE SDL_SCANCODE_GRAVE
#else
	#define DEFAULT_FRAME 4
	#define COUNTER_KEYCODE SDL_SCANCODE_F1
#endif

// Colors
SDL_Color black = { 0, 0, 0, 255 };
SDL_Color red = { 255, 0, 0, 255 };
SDL_Color green = { 0, 210, 60, 255 };
SDL_Color yellow = { 255, 255, 0, 255 };
SDL_Color white = { 255, 255, 255, 255 };

// Fonts
TTF_Font* button_font;
TTF_Font* counter_font;

// Frame values
int frame = DEFAULT_FRAME;
int lastFrame = 0;
int lastGameFrame = 0;
int gameFrame = 1;

// Gravity values
int jumpState = 0;
double gravity = 600.0;
double velocityY = 0.0;
double speed = 200.0;
double jumpStrength = 350.0;
double delta = 0.02;

// Is user a spectator?
bool spectating = false;

// Demo walking direction
bool demo_direction = false;

// Dialog box data
struct DialogBoxData {
	std::string text;
	std::string buttonText;

	DialogBoxData() {
		this->text = "";
		this->buttonText = "OK";
	}

	void Set(std::string text = "", std::string buttonText = "OK") {
		this->text = text;
		this->buttonText = buttonText;
	}
} dialogBox;

void ReopenConsole() {
	#ifdef SYSTEM_WINDOWS
		// Open console if it's not opened
		if(!GetConsoleWindow()) AllocConsole();

		// Allocate stdout and stderr to opened console
		if(GetConsoleWindow()) {
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
		}
	#endif
}

void DisplayError(std::string err) {
	// Write error to stderr
	std::cerr << err << std::endl;

	// Display dialog box with error
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, err.c_str(), NULL);
}

std::string RandomStr(int length, std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
	int charsLen = chars.length() - 1;
	std::string str;
	for(int i = 0; i < length; ++i) {
		str += chars[rand() % charsLen];
	}
	return str;
}

std::string NumToStr(double num, int places = 2) {
	std::string fmt = "%." + std::to_string(places) + "f";
	std::string str = std::string(30, '\0');
	int len = sprintf(&str[0], fmt.c_str(), num);
	str.resize(len);
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

std::string Serialize(int frame, double x, double y) {
	return std::to_string(frame) + " " + std::to_string(x) + " " + std::to_string(y);
}

bool Unserialize(std::string data, int &frame, double &x, double &y) {
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
