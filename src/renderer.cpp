#include "../include/renderer.hpp"

Renderer::Renderer(const char* title, int x, int y, int w, int h) {
	// Init SDL
	if(SDL_Init(SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC)) != 0) {
		this->lastError = "Can't init SDL (" + std::string(SDL_GetError()) + ")";
		return;
	}

	// Create window
	this->window = SDL_CreateWindow(title, x, y, w, h, 0); // SDL_WINDOW_OPENGL
	if(window == NULL) {
		this->lastError = "Can't create window (" + std::string(SDL_GetError()) + ")";
		return;
	}

	// Create renderer
	this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);
	if(renderer == NULL) {
		this->lastError = "Can't create renderer (" + std::string(SDL_GetError()) + ")";
		return;
	}

	// Set window background color
	if(SetColor({ 255, 255, 255, 255 }) != 0) {
		this->lastError = "Can't set window background color (" + std::string(SDL_GetError()) + ")";
		return;
	}

	// Init font engine
	if(TTF_Init() == -1) {
		this->lastError = "Can't init font engine (" + std::string(TTF_GetError()) + ")";
		return;
	}
}

Renderer::~Renderer() {
	SDL_DestroyRenderer(this->renderer);
	SDL_DestroyWindow(this->window);
	TTF_Quit();
	SDL_Quit();
}

void Renderer::RaiseWindow() {
	SDL_RaiseWindow(this->window);
}

int Renderer::SetColor(SDL_Color color) {
	return SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
}

int Renderer::Clear() {
	return SDL_RenderClear(this->renderer);
}

void Renderer::Present() {
	SDL_RenderPresent(this->renderer);
}

int Renderer::Draw(SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect, const double angle, const SDL_Point* center, const SDL_RendererFlip flip) {
	return SDL_RenderCopyEx(this->renderer, texture, srcrect, dstrect, angle, center, flip);
}

SDL_Texture* Renderer::LoadTexture(const char* path) {
	// Load image to surface
	SDL_Surface* surface = IMG_Load(path);
	if(surface == NULL) return NULL;

	// Convert surface to texture
	SDL_Texture* img = SDL_CreateTextureFromSurface(this->renderer, surface);

	// Destroy surface
	SDL_FreeSurface(surface);

	return img;
}

SDL_Texture* Renderer::LoadTexture(SDL_RWops* data) {
	// Load image to surface
	SDL_Surface* surface = IMG_Load_RW(data, 1);
	if(surface == NULL) return NULL;

	// Convert surface to texture
	SDL_Texture* img = SDL_CreateTextureFromSurface(this->renderer, surface);

	// Destroy surface
	SDL_FreeSurface(surface);

	return img;
}

TTF_Font* Renderer::LoadFont(const char* path, int size) {
	return TTF_OpenFont(path, size);
}

TTF_Font* Renderer::LoadFont(SDL_RWops* data, int size) {
	return TTF_OpenFontRW(data, 1, size);
}

SDL_Texture* Renderer::RenderText(TTF_Font* font, std::string text, SDL_Color color) {
	// Render surface with text
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
	if(surface == NULL) {
		return NULL;
	}

	// Convert surface to texture
	SDL_Texture* render = SDL_CreateTextureFromSurface(this->renderer, surface);

	// Destroy surface
	SDL_FreeSurface(surface);

	return render;
}

SDL_Texture* Renderer::RenderText_Solid(TTF_Font* font, std::string text, SDL_Color color) {
	// Render surface with text
	SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
	if(surface == NULL) {
		return NULL;
	}

	// Convert surface to texture
	SDL_Texture* render = SDL_CreateTextureFromSurface(this->renderer, surface);

	// Destroy surface
	SDL_FreeSurface(surface);

	return render;
}

SDL_Texture* Renderer::CreateOverlay(int w, int h, SDL_Color color) {
	// Create texture
	SDL_Texture* overlay = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
	if(overlay == NULL) return NULL;

	// Set texture transparency
	SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);

	// Set render target to texture
	SDL_SetRenderTarget(this->renderer, overlay);

	// Set overlay color
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(this->renderer);

	// Reset render target
	SDL_SetRenderTarget(this->renderer, NULL);

	return overlay;
}

bool Renderer::DrawTriangle(int x, int y, SDL_Color color, int size, int direction) {
	// Set dimensions
	SDL_Point p[4];
	switch(direction) {
		case TRIANGLE_UP:
			p[0] = { x + size / 2, y };
			p[1] = { x + size - 1, y + size - 1 };
			p[2] = { x, y + size - 1 };
			p[3] = { x + size / 2, y };
			break;
		case TRIANGLE_DOWN:
			p[0] = { x, y };
			p[1] = { x + size - 1, y };
			p[2] = { x + size / 2, y + size - 1 };
			p[3] = { x, y };
			break;
		case TRIANGLE_LEFT:
			p[0] = { x + size - 1, y };
			p[1] = { x + size - 1, y + size - 1 };
			p[2] = { x, y + size / 2 };
			p[3] = { x + size - 1, y };
			break;
		case TRIANGLE_RIGHT:
			p[0] = { x, y };
			p[1] = { x + size - 1, y + size / 2 };
			p[2] = { x, y + size - 1 };
			p[3] = { x, y };
			break;
		default:
			return false;
	}

	// Set triangle color
	if(SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a) != 0) {
		return false;
	}

	// Create triangle
	if(SDL_RenderDrawLines(this->renderer, p, 4) != 0) {
		return false;
	}

	// Set render color to default
	if(SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255) != 0) {
		return false;
	}

	return true;
}

bool Renderer::DrawButton(TTF_Font* font, std::string text, SDL_Rect rect, SDL_Color font_color,
			SDL_Color bg_color, SDL_Color border_color, int padding_x, int padding_y, int border_size) {
	// Create button text
	SDL_Texture* rendered_text = RenderText(font, text, font_color);
	if(rendered_text == NULL) return false;

	// Get button text size
	int w, h;
	if(SDL_QueryTexture(rendered_text, NULL, NULL, &w, &h) != 0) {
		return false;
	}

	// If user dimensions is too small, use automatic dimensions
	if(rect.w < w + border_size + 1 + padding_x) rect.w = w + border_size + 1 + padding_x;
	if(rect.h < h + border_size + 1 + padding_y) rect.h = h + border_size + 1 + padding_y;

	// Set border color
	if(SDL_SetRenderDrawColor(this->renderer, border_color.r, border_color.g, border_color.b, border_color.a) != 0) {
		return false;
	}

	// Render border
	if(SDL_RenderFillRect(this->renderer, &rect) != 0) {
		return false;
	}

	// Set background color
	if(SDL_SetRenderDrawColor(this->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a) != 0) {
		return false;
	}

	// Set button background size and position
	rect.x = rect.x + border_size;
	rect.y = rect.y + border_size;
	rect.w = rect.w - border_size * 2;
	rect.h = rect.h - border_size * 2;

	// Render button background
	if(SDL_RenderFillRect(this->renderer, &rect) != 0) {
		return false;
	}

	// Calculate button text position
	rect.x = rect.x + (rect.w - w) / 2;
	rect.y = rect.y + (rect.h - h) / 2;
	rect.w = w;
	rect.h = h;

	// Render button text
	if(SDL_RenderCopy(this->renderer, rendered_text, NULL, &rect) != 0) {
		return false;
	}

	// Set render color to default
	if(SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255) != 0) {
		return false;
	}

	// Destroy button text
	SDL_DestroyTexture(rendered_text);

	return true;
}
