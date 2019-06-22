#ifndef __RENDERER_HPP
#define __RENDERER_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>

// Defines used by DrawTriangle function
#define TRIANGLE_UP 1
#define TRIANGLE_DOWN 2
#define TRIANGLE_LEFT 3
#define TRIANGLE_RIGHT 4

class Renderer {
public:
	SDL_Window* window;
	SDL_Renderer* renderer;
	std::string lastError;

	Renderer(const char* title, int x, int y, int w, int h);
	~Renderer();
	void RaiseWindow();
	int SetColor(SDL_Color color);
	int Clear();
	void Present();
	int Draw(SDL_Texture* texture, const SDL_Rect* srcrect = NULL, const SDL_Rect* dstrect = NULL,
		const double angle = 0, const SDL_Point* center = NULL, const SDL_RendererFlip flip = SDL_FLIP_NONE);
	SDL_Texture* LoadTexture(const char* path);
	SDL_Texture* LoadTexture(SDL_RWops* data);
	TTF_Font* LoadFont(const char* path, int size);
	TTF_Font* LoadFont(SDL_RWops* data, int size);
	SDL_Texture* RenderText(TTF_Font* font, std::string text, SDL_Color color);
	SDL_Texture* RenderText_Solid(TTF_Font* font, std::string text, SDL_Color color);
	SDL_Texture* CreateOverlay(int w, int h, SDL_Color color = { 0, 0, 0, 100 });
	bool DrawTriangle(int x, int y, SDL_Color color, int size, int direction = TRIANGLE_UP);
	bool DrawButton(TTF_Font* font, std::string text, SDL_Rect rect, SDL_Color font_color,
		SDL_Color bg_color, SDL_Color border_color, int padding_x = 14, int padding_y = 6, int border_size = 3);
};

#endif
