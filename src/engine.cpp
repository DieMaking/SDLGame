#include "../include/engine.hpp"

Engine::Engine(const char* title, int x, int y, int w, int h) {
	this->title = title;
	this->wx = x;
	this->wy = y;
	this->ww = w;
	this->wh = h;
}

Engine::~Engine() {
	SDL_DestroyRenderer(this->r);
	SDL_DestroyWindow(this->w);
	#ifndef __EMSCRIPTEN__
		Mix_CloseAudio();
	#endif
	TTF_Quit();
	SDL_Quit();
}

bool Engine::Init() {
	// Init SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		this->lastError = "Can't init SDL (" + std::string(SDL_GetError()) + ")";
		return false;
	}

	// Create window
	this->w = SDL_CreateWindow(this->title, this->wx, this->wy, this->ww, this->wh, SDL_WINDOW_SHOWN); // SDL_WINDOW_OPENGL
	if(this->w == NULL) {
		SDL_Quit();
		this->lastError = "Can't create window (" + std::string(SDL_GetError()) + ")";
		return false;
	}

	#ifndef __EMSCRIPTEN__
		// Get window info
		SDL_VERSION(&this->i.version);
		if(!SDL_GetWindowWMInfo(this->w, &this->i)) {
			SDL_DestroyWindow(this->w);
			SDL_Quit();
			this->lastError = "Can't get window info (" + std::string(SDL_GetError()) + ")";
			return false;
		}
	#endif

	// Create renderer
	this->r = SDL_CreateRenderer(this->w, -1, SDL_RENDERER_ACCELERATED);
	if(this->r == NULL) {
		SDL_DestroyWindow(this->w);
		SDL_Quit();
		this->lastError = "Can't create renderer (" + std::string(SDL_GetError()) + ")";
		return false;
	}

	// Set window background color
	if(this->SetColor({ 255, 255, 255, 255 }) < 0) {
		SDL_DestroyRenderer(this->r);
		SDL_DestroyWindow(this->w);
		SDL_Quit();
		this->lastError = "Can't set window background color (" + std::string(SDL_GetError()) + ")";
		return false;
	}

	// Init font engine
	if(TTF_Init() < 0) {
		SDL_DestroyRenderer(this->r);
		SDL_DestroyWindow(this->w);
		SDL_Quit();
		this->lastError = "Can't init font engine (" + std::string(TTF_GetError()) + ")";
		return false;
	}

	#ifndef __EMSCRIPTEN__
		// Init sound engine
		if(Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
			SDL_DestroyRenderer(this->r);
			SDL_DestroyWindow(this->w);
			TTF_Quit();
			SDL_Quit();
			this->lastError = "Can't init sound engine (" + std::string(Mix_GetError()) + ")";
			return false;
		}
	#endif

	return true;
}

void Engine::ShowWindow(bool show) {
	(show ? SDL_ShowWindow : SDL_HideWindow)(this->w);
}

void Engine::RaiseWindow() {
	SDL_RaiseWindow(this->w);
}

int Engine::SetColor(SDL_Color color) {
	return SDL_SetRenderDrawColor(this->r, color.r, color.g, color.b, color.a);
}

int Engine::SetTarget(SDL_Texture* target) {
	return SDL_SetRenderTarget(this->r, target);
}

int Engine::Clear() {
	return SDL_RenderClear(this->r);
}

void Engine::Present() {
	SDL_RenderPresent(this->r);
}

int Engine::DrawLine(int x, int y, int w, int h) {
	return SDL_RenderDrawLine(this->r, x, y, x + w - 1, y + h - 1);
}

int Engine::Draw(SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect, const double angle, const SDL_Point* center, const SDL_RendererFlip flip) {
	return SDL_RenderCopyEx(this->r, texture, srcrect, dstrect, angle, center, flip);
}

int Engine::QueryTexture(SDL_Texture* txt, SDL_Rect* rect, uint32_t* format, int* access) {
	return SDL_QueryTexture(txt, format, access, &rect->w, &rect->h);
}

SDL_Texture* Engine::CreateTexture(int w, int h, int access) {
	return SDL_CreateTexture(this->r, SDL_PIXELFORMAT_RGBA8888, access, w, h);
}

SDL_Texture* Engine::ConnectTextures(SDL_Texture* txt1, SDL_Texture* txt2, int method, bool destroy) {
	if(txt1 == NULL || txt2 == NULL) return NULL;

	SDL_Rect r1;
	if(this->QueryTexture(txt1, &r1) < 0) return NULL;

	SDL_Rect r2;
	if(this->QueryTexture(txt2, &r2) < 0) return NULL;

	int w = 0, h = 0;
	switch(method) {
		case CONNECT_1ON2:
		case CONNECT_2ON1:
			w = (r2.w < r1.w ? r2.w : r1.w);
			h = (r2.h < r1.h ? r2.h : r1.h);
			break;
		case CONNECT_1LEFT2:
		case CONNECT_2LEFT1:
		case CONNECT_1RIGHT2:
		case CONNECT_2RIGHT1:
			w = r1.w + r2.w;
			h = (r2.h > r1.h ? r2.h : r1.h);
			break;
		case CONNECT_1TOP2:
		case CONNECT_2TOP1:
		case CONNECT_1BOTTOM2:
		case CONNECT_2BOTTOM1:
			w = (r2.w > r1.w ? r2.w : r1.w);
			h = r1.h + r2.h;
			break;
	}

	SDL_Texture* target = this->CreateTexture(w, h, SDL_TEXTUREACCESS_TARGET);
	if(target == NULL) return NULL;
	this->SetTarget(target);
	this->Clear();

	switch(method) {
		case CONNECT_1ON2:
			this->Draw(txt2);
			this->Draw(txt1);
			break;
		case CONNECT_2ON1:
			this->Draw(txt1);
			this->Draw(txt2);
			break;
		case CONNECT_1LEFT2:
		case CONNECT_2RIGHT1:
			r1.x = 0;
			r1.y = 0;
			this->Draw(txt1, NULL, &r1);
			r2.x = r1.w;
			r2.y = 0;
			this->Draw(txt2, NULL, &r2);
			break;
		case CONNECT_2LEFT1:
		case CONNECT_1RIGHT2:
			r2.x = 0;
			r2.y = 0;
			this->Draw(txt2, NULL, &r2);
			r1.x = r2.w;
			r1.y = 0;
			this->Draw(txt1, NULL, &r1);
			break;
		case CONNECT_1TOP2:
		case CONNECT_2BOTTOM1:
			r1.x = 0;
			r1.y = 0;
			this->Draw(txt1, NULL, &r1);
			r2.x = 0;
			r2.y = r1.h;
			this->Draw(txt2, NULL, &r2);
			break;
		case CONNECT_2TOP1:
		case CONNECT_1BOTTOM2:
			r2.x = 0;
			r2.y = 0;
			this->Draw(txt2, NULL, &r2);
			r1.x = 0;
			r1.y = r2.h;
			this->Draw(txt1, NULL, &r1);
			break;
	}

	this->SetTarget(NULL);
	if(destroy) {
		SDL_DestroyTexture(txt1);
		SDL_DestroyTexture(txt2);
	}
	return target;
}

SDL_Texture* Engine::SurfaceToTexture(SDL_Surface* surface) {
	SDL_Texture* out = SDL_CreateTextureFromSurface(this->r, surface);
	if(out != NULL) {
		SDL_FreeSurface(surface);
		surface = NULL;
	}
	return out;
}

SDL_Texture* Engine::LoadTexture(const char* path) {
	SDL_Surface* surface = IMG_Load(path);
	return (surface != NULL ? this->SurfaceToTexture(surface) : NULL);
}

SDL_Texture* Engine::LoadTexture(SDL_RWops* data) {
	SDL_Surface* surface = IMG_Load_RW(data, 1);
	return (surface != NULL ? this->SurfaceToTexture(surface) : NULL);
}

#ifndef __EMSCRIPTEN__
	Mix_Chunk* Engine::LoadSound(const char* path) {
		return Mix_LoadWAV(path);
	}

	Mix_Chunk* Engine::LoadSound(SDL_RWops* data) {
		return Mix_LoadWAV_RW(data, 1);
	}
#endif

TTF_Font* Engine::LoadFont(const char* path, int size) {
	return TTF_OpenFont(path, size);
}

TTF_Font* Engine::LoadFont(SDL_RWops* data, int size) {
	return TTF_OpenFontRW(data, 1, size);
}

SDL_Texture* Engine::RenderText(TTF_Font* font, std::string text, SDL_Color color) {
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
	return (surface != NULL ? this->SurfaceToTexture(surface) : NULL);
}

SDL_Texture* Engine::RenderSolidText(TTF_Font* font, std::string text, SDL_Color color) {
	SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
	return (surface != NULL ? this->SurfaceToTexture(surface) : NULL);
}

SDL_Texture* Engine::CreateOverlay(int w, int h, SDL_Color color) {
	// Create texture
	SDL_Texture* overlay = this->CreateTexture(w, h, SDL_TEXTUREACCESS_TARGET);
	if(overlay != NULL) {
		// Set texture transparency
		SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);

		// Set render target to texture
		this->SetTarget(overlay);

		// Set overlay color
		this->SetColor(color);
		this->Clear();

		// Reset render target
		this->SetTarget(NULL);
	}
	return overlay;
}

bool Engine::DrawTriangle(int x, int y, SDL_Color color, int size, int direction) {
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
	if(this->SetColor(color) < 0) {
		return false;
	}

	// Create triangle
	if(SDL_RenderDrawLines(this->r, p, 4) < 0) {
		return false;
	}

	// Set render color to default
	if(this->SetColor({ 255, 255, 255, 255 }) < 0) {
		return false;
	}

	return true;
}

bool Engine::DrawButton(TTF_Font* font, std::string text, SDL_Rect rect, SDL_Color font_color,
			SDL_Color bg_color, SDL_Color border_color, int padding_x, int padding_y, int border_size) {
	// Create button text
	SDL_Texture* rendered_text = this->RenderText(font, text, font_color);
	if(rendered_text == NULL) return false;

	// Get button text size
	int w, h;
	if(SDL_QueryTexture(rendered_text, NULL, NULL, &w, &h) < 0) {
		return false;
	}

	// If user dimensions is too small, use automatic dimensions
	if(rect.w < w + border_size + 1 + padding_x) rect.w = w + border_size + 1 + padding_x;
	if(rect.h < h + border_size + 1 + padding_y) rect.h = h + border_size + 1 + padding_y;

	// Set border color
	if(this->SetColor(border_color) < 0) {
		return false;
	}

	// Render border
	if(SDL_RenderFillRect(this->r, &rect) < 0) {
		return false;
	}

	// Set background color
	if(this->SetColor(bg_color) < 0) {
		return false;
	}

	// Set button background size and position
	rect.x = rect.x + border_size;
	rect.y = rect.y + border_size;
	rect.w = rect.w - border_size * 2;
	rect.h = rect.h - border_size * 2;

	// Render button background
	if(SDL_RenderFillRect(this->r, &rect) < 0) {
		return false;
	}

	// Calculate button text position
	rect.x = rect.x + (rect.w - w) / 2;
	rect.y = rect.y + (rect.h - h) / 2;
	rect.w = w;
	rect.h = h;

	// Render button text
	if(this->Draw(rendered_text, NULL, &rect) < 0) {
		return false;
	}

	// Set render color to default
	if(this->SetColor({ 255, 255, 255, 255 }) < 0) {
		return false;
	}

	// Destroy button text
	SDL_DestroyTexture(rendered_text);

	return true;
}

/*SDL_Texture* Engine::DrawButton(TTF_Font* font, std::string text, SDL_Rect rect, SDL_Color font_color,
			SDL_Color bg_color, SDL_Color border_color, int padding_x, int padding_y, int border_size) {
	// Create button text
	SDL_Texture* rendered_text = RenderText(font, text, font_color);
	if(rendered_text == NULL) return false;

	// Get button text size
	int w, h;
	if(SDL_QueryTexture(rendered_text, NULL, NULL, &w, &h) < 0) {
		return false;
	}

	// If user dimensions is too small, use automatic dimensions
	if(rect.w < w + border_size + 1 + padding_x) rect.w = w + border_size + 1 + padding_x;
	if(rect.h < h + border_size + 1 + padding_y) rect.h = h + border_size + 1 + padding_y;

	// Set render target to texture
	SDL_Texture* out = SDL_CreateTexture(this->r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, rect.w, rect.h);
	if(SDL_SetRenderTarget(this->r, out) < 0) return false;

	// Set border color
	if(SDL_SetRenderDrawColor(this->r, border_color.r, border_color.g, border_color.b, border_color.a) < 0) {
		return false;
	}

	// Render border
	if(SDL_RenderFillRect(this->r, &rect) < 0) {
		return false;
	}

	// Set background color
	if(SDL_SetRenderDrawColor(this->r, bg_color.r, bg_color.g, bg_color.b, bg_color.a) < 0) {
		return false;
	}

	// Set button background size and position
	rect.x = rect.x + border_size;
	rect.y = rect.y + border_size;
	rect.w = rect.w - border_size * 2;
	rect.h = rect.h - border_size * 2;

	// Render button background
	if(SDL_RenderFillRect(this->r, &rect) < 0) {
		return false;
	}

	// Calculate button text position
	rect.x = rect.x + (rect.w - w) / 2;
	rect.y = rect.y + (rect.h - h) / 2;
	rect.w = w;
	rect.h = h;

	// Render button text
	if(SDL_RenderCopy(this->r, rendered_text, NULL, &rect) < 0) {
		return false;
	}

	// Set render target to renderer
	if(SDL_SetRenderTarget(this->r, out) < 0) {
		return false;
	}

	// Set render color to default
	if(SDL_SetRenderDrawColor(this->r, 255, 255, 255, 255) < 0) {
		return false;
	}

	// Destroy button text
	SDL_DestroyTexture(rendered_text);

	return true;
}*/
