#ifndef __ENGINE_HPP
#define __ENGINE_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_syswm.h>
#include <string>

enum { // Used by DrawTriangle function
	TRIANGLE_UP, TRIANGLE_DOWN,
	TRIANGLE_LEFT, TRIANGLE_RIGHT
};

class Engine {
private:
	const char* title;
	int wx, wy, ww, wh;
public:
	SDL_Window* w;
	SDL_Renderer* r;
	SDL_SysWMinfo i;
	std::string lastError;

	Engine(const char* title, int x, int y, int w, int h);
	~Engine();
	bool Init();
	void ShowWindow(bool show = true);
	void RaiseWindow();
	int SetColor(SDL_Color color);
	int SetTarget(SDL_Texture* target);
	int Clear();
	void Present();
	int DrawLine(int x, int y, int w, int h);
	int Draw(SDL_Texture* texture, const SDL_Rect* srcrect = NULL, const SDL_Rect* dstrect = NULL,
		const double angle = 0, const SDL_Point* center = NULL, const SDL_RendererFlip flip = SDL_FLIP_NONE);
	SDL_Texture* CreateTexture(int w, int h, int access = SDL_TEXTUREACCESS_STATIC);
	SDL_Texture* SurfaceToTexture(SDL_Surface* surface);
	SDL_Texture* LoadTexture(const char* path);
	SDL_Texture* LoadTexture(SDL_RWops* data);
	Mix_Chunk* LoadSound(const char* path);
	Mix_Chunk* LoadSound(SDL_RWops* data);
	TTF_Font* LoadFont(const char* path, int size);
	TTF_Font* LoadFont(SDL_RWops* data, int size);
	SDL_Texture* RenderText(TTF_Font* font, std::string text, SDL_Color color);
	SDL_Texture* RenderSolidText(TTF_Font* font, std::string text, SDL_Color color);
	SDL_Texture* CreateOverlay(int w, int h, SDL_Color color = { 0, 0, 0, 100 });
	bool DrawTriangle(int x, int y, SDL_Color color, int size, int direction = TRIANGLE_UP);
	bool DrawButton(TTF_Font* font, std::string text, SDL_Rect rect, SDL_Color font_color,
		SDL_Color bg_color, SDL_Color border_color, int padding_x = 14, int padding_y = 6, int border_size = 3);
};

#endif
