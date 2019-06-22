#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <ctime>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include "../include/files.hpp"
#else
	#include <discord_rpc.h>
	#include "../include/easysock/tcp.hpp"
	#include "../include/discord_rpc.hpp"
#endif

#include "../include/renderer.hpp"
#include "../include/game.hpp"

/*
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	Uint32 rmask = 0xff000000;
	Uint32 gmask = 0x00ff0000;
	Uint32 bmask = 0x0000ff00;
	Uint32 amask = 0x000000ff;
#else
	Uint32 rmask = 0x000000ff;
	Uint32 gmask = 0x0000ff00;
	Uint32 bmask = 0x00ff0000;
	Uint32 amask = 0xff000000;
#endif
*/

// Key locks
bool escKey;
bool optKey;
bool enterKey;
bool jumpKey;
bool counterKey;
bool mouseLock;

// Main menu option
int option;

// Main menu switch
bool playing;

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
Uint32 mouse;

// Last X and Y
int lastX, lastY;

// Used to count FPS
uint32_t fpsFrameTicks;
uint32_t fpsFrames;

// Counter switch
bool showCounter;

// Resources used in main loop
SDL_Texture* bg;
SDL_Texture* menubg;
SDL_Texture* player;
SDL_Texture* overlay;
SDL_Texture* fps_counter;
SDL_Texture* text;
SDL_Texture* counter1;
SDL_Texture* counter2;
SDL_Texture* counter3;
SDL_Texture* counter4;
SDL_Texture* counter5;

// Game main functions
void MainLoop();
void Frame();

#ifndef __EMSCRIPTEN__
	// Used to count delay between frames
	uint32_t frameTicks;

	// Used to end main loop
	bool quit;

	// Server connection
	easysock::tcp::Client* conn;
	bool connected;

	// Startup thread
	pthread_t thread;
	void* StartupThread(void*);
#endif

Renderer renderer(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height);

int main(int argc, char* argv[]) {
	// Needed by rand() function
	srand(time(0));

	// Set default values
	option = 1;

	#ifndef __EMSCRIPTEN__
		// Arguments loop
		for(int i = 1; i < argc; i++) {
			std::string arg = argv[i];

			// Enable debug
			if(arg == "--debug" && !debug) {
				showCounter = true;
				debug = true;
				ReopenConsole();
			}
			if(arg == "--demo" && !demo) {
				demo = true;
			}
		}
	#endif

	if(!renderer.lastError.empty()) {
		DisplayError(renderer.lastError);
		return 1;
	}

	// Load raw resources
	#ifdef __EMSCRIPTEN__
		SDL_RWops* button_font_res = SDL_RWFromConstMem(button_font_raw, button_font_raw_size);
		SDL_RWops* counter_font_res = SDL_RWFromConstMem(counter_font_raw, counter_font_raw_size);
		SDL_RWops* bg_res = SDL_RWFromConstMem(bg_raw, bg_raw_size);
		SDL_RWops* menubg_res = SDL_RWFromConstMem(menubg_raw, menubg_raw_size);
		SDL_RWops* player_res = SDL_RWFromConstMem(player_raw, player_raw_size);
		if(button_font_res == NULL || counter_font_res == NULL || bg_res == NULL || menubg_res == NULL || player_res == NULL) {
			DisplayError("Can't load required assets (" + std::string(SDL_GetError()) + ")");	
			return 1;
		}
	#else
		const char* button_font_res = "fonts/DroidSans.ttf";
		const char* counter_font_res = "fonts/Visitor2.ttf";
		const char* bg_res = "images/bg.png";
		const char* menubg_res = "images/menubg.png";
		const char* player_res = "images/player.png";
	#endif

	// Load fonts
	button_font = renderer.LoadFont(button_font_res, 22);
	counter_font = renderer.LoadFont(counter_font_res, 25);
	if(button_font == NULL || counter_font == NULL) {
		DisplayError("Can't load required assets (" + std::string(TTF_GetError()) + ")");
		return 1;
	}

	// Load textures
	bg = renderer.LoadTexture(bg_res);
	menubg = renderer.LoadTexture(menubg_res);
	player = renderer.LoadTexture(player_res);
	if(bg == NULL || menubg == NULL || player == NULL) {
		DisplayError("Can't load required assets (" + std::string(IMG_GetError()) + ")");
		return 1;
	}

	// Create overlay
	overlay = renderer.CreateOverlay(width, height);

	// Create FPS counter
	fps_counter = renderer.RenderText_Solid(counter_font, "FPS: 0", black);

	#ifdef __EMSCRIPTEN__
		// Run main loop
		emscripten_set_main_loop(&MainLoop, 0, 1);
	#else
		// Init easysock (needed on Windows)
		easysock::init();

		// Init Discord RPC
		DiscordRPC::Init("411983281886593024");

		// Update Discord RPC
		DiscordRPC::MatchSecret = RandomStr(24);
		DiscordRPC::JoinSecret = RandomStr(24);
		DiscordRPC::SpectateSecret = RandomStr(24);
		DiscordRPC::StartTimestamp = time(0);
		DiscordRPC::LargeImageKey = "square_icon";
		DiscordRPC::LargeImageText = version;
		DiscordRPC::UpdatePresence();

		if(!demo) {// Run startup thread
			pthread_create(&thread, NULL, StartupThread, NULL);
		} else { // Set up demo mode
			playing = true;
			frame = 1;
		}

		while(!quit) {
			// Get ticks at frame start
			frameTicks = SDL_GetTicks();

			// Run main loop
			MainLoop();

			if(!quit) {
				// Wait remaining time
				frameTicks = SDL_GetTicks() - frameTicks;
				if(frameTicks < 1000 / fps) {
					SDL_Delay(1000 / fps - frameTicks);
				}
			}
		}
	#endif

	return 0;
}

void MainLoop() {
	// Exitting frame
	if(frame == 0) {
		#ifdef __EMSCRIPTEN__
			// Set render color to black
			renderer.SetColor({ 0, 0, 0, 255 });

			// Clear renderer
			renderer.Clear();

			// Show render
			renderer.Present();
		#else
			// Quit Discord
			Discord_Shutdown();

			// Quit easysock (needed on Windows)
			easysock::exit();
		#endif

		// Destroy resources
		SDL_DestroyTexture(bg);
		SDL_DestroyTexture(player);
		SDL_DestroyTexture(overlay);
		SDL_DestroyTexture(fps_counter);

		// Destroy fonts
		TTF_CloseFont(button_font);
		TTF_CloseFont(counter_font);

		#ifdef __EMSCRIPTEN__
			// Run JavaScript callback
			EM_ASM(
				if(sdlgame_on_exit) sdlgame_on_exit();
			);

			// Exit from the loop
			emscripten_cancel_main_loop();
		#else
			// Exit from the loop
			quit = true;
		#endif

		return;
	}

	// Load frame
	if(lastFrame != frame) {
		lastFrame = frame;
		switch(frame) {
			case 1: // Game
				#ifndef __EMSCRIPTEN__
					// Update Discord Presence
					DiscordRPC::State = "In Game";
					DiscordRPC::Details = (spectating ? "Spectating someone" : (demo ? "Watching demo" : "Stage 1"));
					DiscordRPC::UpdatePresence();
				#endif

				break;
			case 2: // Main menu
				#ifndef __EMSCRIPTEN__
					// Update Discord Presence
					DiscordRPC::State = "In Main Menu";
					DiscordRPC::Details = "Wasting some time";
					DiscordRPC::UpdatePresence();
				#endif

				break;
			case 3: // Options
				// Create resources
				text = renderer.RenderText(button_font, "Coming soon...", white);

				#ifndef __EMSCRIPTEN__
					// Update Discord Presence
					DiscordRPC::State = "In Main Menu";
					DiscordRPC::Details = "Changing some options";
					DiscordRPC::UpdatePresence();
				#endif

				break;
			case 4: // Dialog box
				#ifndef __EMSCRIPTEN__
					// Update Discord Presence
					DiscordRPC::State = "In Main Menu";
					DiscordRPC::Details = "Wasting some time";
					DiscordRPC::UpdatePresence();
				#endif

				break;
		}
	}

	// Run frame
	Frame();

	// Unload frame (frame end)
	switch(lastFrame) {
		case 1: // Game
			// Destroy counter lines
			SDL_DestroyTexture(counter1);
			SDL_DestroyTexture(counter2);
			SDL_DestroyTexture(counter3);
			SDL_DestroyTexture(counter4);
			SDL_DestroyTexture(counter5);
			break;
		case 2: // Main menu
			//
			break;
		case 3: // Options
			//
			break;
		case 4: // Dialog box
			// Destroy text
			SDL_DestroyTexture(text);
			break;
	}

	// Unload frame (frame change)
	if(lastFrame != frame) {
		switch(lastFrame) {
			case 1: // Game
				//
				break;
			case 2: // Main menu
				//
				break;
			case 3: // Options
				// Destroy resources
				SDL_DestroyTexture(text);
				break;
			case 4: // Dialog box
				//
				break;
		}
	}
}

void Frame() {
	// FPS counting
	fpsFrames++;
	if(fpsFrameTicks + 1000 < SDL_GetTicks()) {
		fps_counter = renderer.RenderText_Solid(counter_font, "FPS: " + std::to_string(fpsFrames), black);
		fpsFrames = 0;
		fpsFrameTicks = SDL_GetTicks();
	}

	// Events
	if(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT) {
			frame = 0;
			return;
		}
	}

	#ifndef __EMSCRIPTEN__
		// Run Discord Presence tasks
		DiscordRPC::RunTasks();
	#endif

	// Keyboard handling
	key = SDL_GetKeyboardState(NULL);

	// Mouse handling
	mouse = SDL_GetMouseState(&mouseX, &mouseY);

	// Reset key locks
	if(!key[SDL_SCANCODE_ESCAPE] && escKey) {
		escKey = false;
	}
	if(!key[SDL_SCANCODE_DOWN] && !key[SDL_SCANCODE_UP] && optKey) {
		optKey = false;
	}
	if(!key[SDL_SCANCODE_RETURN] && enterKey) {
		enterKey = false;
	}
	if(!key[SDL_SCANCODE_UP] && jumpKey) {
		jumpKey = false;
	}
	if(!key[COUNTER_KEYCODE] && counterKey) {
		counterKey = false;
	}
	if(!(mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) && mouseLock) {
		mouseLock = false;
	}

	// Show/hide counter
	if(key[COUNTER_KEYCODE] && !counterKey) {
		counterKey = true;
		showCounter = !showCounter;
	}

	// Show main menu, back to game or exit
	if(key[SDL_SCANCODE_ESCAPE] && !escKey) {
		escKey = true;
		if(frame != 4 || !dialogBox.buttonText.empty()) {
			frame = (demo ? 0 : (frame != 2 ? 2 : (playing ? 1 : 0)));
		}
		return;
	}

	switch(frame) {
		case 1: // Game
			// If not spectating and not in demo mode
			if(!spectating && !demo) {
				// Move right and left
				if(key[SDL_SCANCODE_LEFT]) {
					if(flip != SDL_FLIP_HORIZONTAL) flip = SDL_FLIP_HORIZONTAL;
					posX -= speed * delta;
				}
				if(key[SDL_SCANCODE_RIGHT]) {
					if(flip != SDL_FLIP_NONE) flip = SDL_FLIP_NONE;
					posX += speed * delta;
				}

				// Jump
				if(key[SDL_SCANCODE_UP] && !jumpKey && jumpState < 2) {
					jumpKey = true;
					jumpState++;
					velocityY = -(jumpState == 2 ? jumpStrength * 2 : jumpStrength);
				}
				if(!key[SDL_SCANCODE_UP] && jumpKey) {
					jumpKey = false;
				}

				// Gravity
				posY += velocityY * delta;

				// Falling
				if(posY < height - sizeY) {
					if(jumpState != 3 && velocityY > 200) {
						jumpState = 3;
					}
					velocityY += gravity * delta;
				} else if(jumpState > 0) {
					jumpKey = false;
					jumpState = 0;
					velocityY = 0;
				}

				// Move character if it's below bottom barrier of the window
				if(posY > height - sizeY) {
					posY = height - sizeY;
				}

				// Go to next frame OR stop player on edge of window
				if(gameFrame + 1 <= gameFrames) {
					if(posX >= width - sizeX / 2) {
						gameFrame++;
						posX = sizeX / 2 - sizeX + 1;
					}
				} else if(posX > width - sizeX) {
					posX = width - sizeX;
				}

				// Go to previous frame OR stop player on edge of window
				if(gameFrame - 1 > 0) {
					if(posX <= sizeX / 2 - sizeX) {
						gameFrame--;
						posX = width - sizeX / 2 - 1;
					}
				} else if(posX < 1) {
					posX = 1;
				}

				// Send player position to the server
				if(connected && !spectating && (posX != lastX || posY != lastY)) {
					lastX = posX;
					lastY = posY;
					if(conn->write("send " + Serialize(gameFrame, posX, posY) + "|") < 0) {
						frame = 4;
						dialogBox.Set("Error while communicating with the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
						pthread_create(&thread, NULL, StartupThread, NULL);
						return;
					}
				}

				// Get player position from the server (if spectating)
				if(connected && spectating) {
					std::string status = conn->read();
					if(easysock::lastError) {
						dialogBox.Set("Error while communicating with the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
						spectating = false;
						pthread_create(&thread, NULL, StartupThread, NULL);
						return;
					} else {
						Unserialize(status, gameFrame, posX, posY);
					}
				}

				// On game frame change
				if(gameFrame != lastGameFrame) {
					lastGameFrame = gameFrame;

					#ifndef __EMSCRIPTEN__
						// Update Discord Presence
						DiscordRPC::Details = "Stage " + std::to_string(gameFrame);
						DiscordRPC::UpdatePresence();
					#endif
				}
			} else if(demo) {
				if(demo_direction) { // Left
					if(flip != SDL_FLIP_HORIZONTAL) flip = SDL_FLIP_HORIZONTAL;
					posX -= speed * delta;
				} else { // Right
					if(flip != SDL_FLIP_NONE) flip = SDL_FLIP_NONE;
					posX += speed * delta;
				}

				// Gravity
				posY += velocityY * delta;

				// Jumping/Falling
				if(posY < height - sizeY) {
					if(jumpState == 1 && velocityY > 100) {
						velocityY = -(jumpStrength * 2);
						jumpState++;
					}
					if(jumpState != 3 && velocityY > 200) {
						jumpState = 3;
					}
					velocityY += gravity * delta;
				} else if(jumpState > 0) {
					jumpState = 0;
					velocityY = 0;
				} else {
					jumpState++;
					velocityY = -jumpStrength;
				}

				// Move character if it's below bottom barrier of the window
				if(posY > height - sizeY) {
					posY = height - sizeY;
				}

				// Go to next frame OR stop player on edge of window
				if(gameFrame + 1 <= gameFrames) {
					if(posX >= width - sizeX / 2) {
						gameFrame++;
						posX = sizeX / 2 - sizeX + 1;
					}
				} else if(posX > width - sizeX) {
					posX = width - sizeX;
					if(!demo_direction) demo_direction = true;
				}

				// Go to previous frame OR stop player on edge of window
				if(gameFrame - 1 > 0) {
					if(posX <= sizeX / 2 - sizeX) {
						gameFrame--;
						posX = width - sizeX / 2 - 1;
					}
				} else if(posX < 1) {
					posX = 1;
					if(demo_direction) demo_direction = false;
				}
			}

			// Reload counter
			if(showCounter) {
				counter1 = renderer.RenderText_Solid(counter_font, "X: " + NumToStr(posX), black);
				counter2 = renderer.RenderText_Solid(counter_font, "Y: " + NumToStr(posY), black);
				counter3 = renderer.RenderText_Solid(counter_font, "Frame: " + NumToStr(gameFrame) + "/" + NumToStr(gameFrames), black);
				counter4 = renderer.RenderText_Solid(counter_font, "Jump state: " + NumToStr(jumpState), black);
				counter5 = renderer.RenderText_Solid(counter_font, "Velocity: " + NumToStr(velocityY), black);
			}

			break;
		case 2: // Main menu
			// Change option
			if(key[SDL_SCANCODE_UP] && !optKey) {
				optKey = true;
				option--;
				if(option < 1) option = 3;
			}
			if(key[SDL_SCANCODE_DOWN] && !optKey) {
				optKey = true;
				option++;
				if(option > 3) option = 1;
			}

			// Accept
			if(key[SDL_SCANCODE_RETURN] && !enterKey) {
				enterKey = true;
				if(option == 1) { // Play / Back to game
					if(!playing) playing = true;
					frame = 1;
					return;
				} else if(option == 2) { // Options
					frame = 3;
					return;
				} else if(option == 3) { // Exit
					frame = 0;
					return;
				}
			}

			// Hover
			if(mouseX > 300 && mouseX <= 500 && mouseY > 200 && mouseY <= 248 && (mouseX != lastX || mouseY != lastY)) { // Play / Back to game
				option = 1;
				lastX = mouseX;
				lastY = mouseY;
			}
			if(mouseX > 300 && mouseX <= 500 && mouseY > 260 && mouseY <= 308 && (mouseX != lastX || mouseY != lastY)) { // Options
				option = 2;
				lastX = mouseX;
				lastY = mouseY;
			}
			if(mouseX > 300 && mouseX <= 500 && mouseY > 320 && mouseY <= 368 && (mouseX != lastX || mouseY != lastY)) { // Exit
				option = 3;
				lastX = mouseX;
				lastY = mouseY;
			}

			// Click
			if((mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) && !mouseLock) {
				mouseLock = true;
				if(mouseX > 300 && mouseX <= 500 && mouseY > 200 && mouseY <= 248) { // Back to game
					if(!playing) playing = true;
					frame = 1;
					return;
				} else if(mouseX > 300 && mouseX <= 500 && mouseY > 260 && mouseY <= 308) { // Options
					frame = 3;
					return;
				} else if(mouseX > 300 && mouseX <= 500 && mouseY > 320 && mouseY <= 368) { // Exit
					frame = 0;
					return;
				}
			}

			break;
		case 3: // Options
			//
			break;
		case 4: // Dialog box
			// If dialog box buttons is shown
			if(!dialogBox.buttonText.empty()) {
				// Accept
				if(key[SDL_SCANCODE_RETURN] && !enterKey) {
					enterKey = true;
					frame = 2;
					break;
				}

				// Hide dialog box
				if(key[SDL_SCANCODE_ESCAPE] && !escKey) {
					escKey = true;
					frame = 2;
					break;
				}

				// Click
				if((mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) && !mouseLock) {
					mouseLock = true;
					if(mouseX > 300 && mouseX <= 500 && mouseY > 300 && mouseY <= 348) {
						frame = 2;
						break;
					}
				}
			}

			// Render text
			text = renderer.RenderText(button_font, dialogBox.text, white);

			break;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Clear renderer
	renderer.Clear();

	switch(frame) {
		case 1: // Game
			// Render background (flip horizontally if gameFrame % 2 is 0)
			renderer.Draw(bg, NULL, NULL, 0, NULL, gameFrame % 2 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);

			// Render game frames
			switch(gameFrame) {
				case 1:
					// Render right triangle
					renderer.DrawTriangle(width - 110, height / 2 - 100, black, 100, TRIANGLE_RIGHT);
					renderer.DrawTriangle(width - 350, height / 2 - 100, black, 100, TRIANGLE_UP);
					renderer.DrawTriangle(250, height / 2 - 100, black, 100, TRIANGLE_DOWN);
					renderer.DrawTriangle(10, height / 2 - 100, black, 100, TRIANGLE_LEFT);
					break;
				case 2:
					// Render left and light triangle
					renderer.DrawTriangle(10, height / 2 - 100, black, 100, TRIANGLE_LEFT);
					renderer.DrawTriangle(width - 110, height / 2 - 100, black, 100, TRIANGLE_RIGHT);
					break;
				case 3:
					// Render left triangle
					renderer.DrawTriangle(10, height / 2 - 100, black, 100, TRIANGLE_LEFT);
					break;
			}

			// Set player size and position
			rect.x = posX;
			rect.y = posY;
			rect.w = sizeX;
			rect.h = sizeY;

			// Render player
			renderer.Draw(player, NULL, &rect, 0, NULL, flip);

			// If counter is shown
			if(showCounter) {
				// Loop through counter lines
				for(int i = 1; i <= 5; i++) {
					// Set counter line position
					rect.x = 10;
					rect.y = 4 + (18 * i);

					// Get counter line by index
					SDL_Texture* counter = (i == 1 ? counter1 : i == 2 ? counter2 : i == 3 ? counter3 : i == 4 ? counter4 : i == 5 ? counter5 : NULL);

					// Get and set counter line size
					SDL_QueryTexture(counter, NULL, NULL, &rect.w, &rect.h);

					// Render counter line
					renderer.Draw(counter, NULL, &rect);
				}
			}

			break;
		case 2: // Main menu
			// Render background
			renderer.Draw(menubg, NULL, NULL);

			// Render background overlay
			renderer.Draw(overlay, NULL, NULL);

			// Set button size and position
			rect.x = 300;
			rect.y = 200;
			rect.w = 200;
			rect.h = 50;

			// Render button
			renderer.DrawButton(button_font, playing ? "Back to game" : "Play", rect, white, green, option == 1 ? red : green);

			// Set button size and position
			rect.x = 300;
			rect.y = 260;
			rect.w = 200;
			rect.h = 50;

			// Render button
			renderer.DrawButton(button_font, "Options", rect, white, green, option == 2 ? red : green);

			// Set button size and position
			rect.x = 300;
			rect.y = 320;
			rect.w = 200;
			rect.h = 50;

			// Render button
			renderer.DrawButton(button_font, "Exit", rect, white, green, option == 3 ? red : green);

			break;
		case 3: // Options
			// Render background
			renderer.Draw(menubg, NULL, NULL);

			// Render background overlay
			renderer.Draw(overlay, NULL, NULL);

			// Get and set text size
			SDL_QueryTexture(text, NULL, NULL, &rect.w, &rect.h);

			// Set text position
			rect.x = width / 2 - rect.w / 2;
			rect.y = height / 2 - rect.h / 2;

			// Display text
			renderer.Draw(text, NULL, &rect);

			break;
		case 4: // Dialog box
			// Render background
			renderer.Draw(menubg, NULL, NULL);

			// Render background overlay
			renderer.Draw(overlay, NULL, NULL);

			// Get and set text size
			SDL_QueryTexture(text, NULL, NULL, &rect.w, &rect.h);

			// Set text position
			rect.x = width / 2 - rect.w / 2;
			rect.y = 260;

			// Display text
			renderer.Draw(text, NULL, &rect);

			// Set button size and position
			rect.x = 300;
			rect.y = 300;
			rect.w = 200;
			rect.h = 50;

			// If dialog box buttons is shown
			if(!dialogBox.buttonText.empty()) {
				// Render button
				renderer.DrawButton(button_font, dialogBox.buttonText, rect, white, green, red);
			}

			break;
	}

	// Render FPS counter
	if(showCounter) {
		// Set counter position
		rect.x = 10;
		rect.y = 4;

		// Get and set counter size
		SDL_QueryTexture(fps_counter, NULL, NULL, &rect.w, &rect.h);

		// Render counter
		renderer.Draw(fps_counter, NULL, &rect);
	}

	// Show render
	renderer.Present();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __EMSCRIPTEN__
	void* StartupThread(void*) {
		// TODO: Skip button
		dialogBox.Set("Connecting to the server...", "");

		if(connected) {
			delete conn;
			connected = false;
		}

		conn = easysock::tcp::connect("themaking.xyz", 34602);
		if(conn == nullptr) {
			dialogBox.Set("Cannot connect to the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
		} else {
			if(conn->write("connect " + DiscordRPC::SpectateSecret + "|") < 0) {
				dialogBox.Set("Error while communicating with the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
				delete conn;
			} else {
				std::string status = conn->read();
				if(easysock::lastError) {
					dialogBox.Set("Error while communicating with the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
					delete conn;
				} else if(status != "success") {
					if(status == "invalid_token") {
						dialogBox.Set("Invalid token (try restarting your game");
					} else {
						dialogBox.Set("Internal server error");
					}
					delete conn;
				} else {
					connected = true;
					frame = 2;
				}
			}
		}

		pthread_exit(NULL);
		return NULL;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DiscordRPC::OnReady(const DiscordUser* user) {
		if(debug) std::cout << "Discord RPC is ready" << std::endl;
	}

	void DiscordRPC::OnDisconnect(int errcode, const char* message) {
		DisplayError("[" + std::to_string(errcode) + "] Disconnected from Discord RPC: " + std::string(message));
		TTF_CloseFont(button_font);
		TTF_CloseFont(counter_font);
		exit(1);
	}

	void DiscordRPC::OnError(int errcode, const char* message) {
		DisplayError("[" + std::to_string(errcode) + "] Discord RPC Error: " + std::string(message));
		TTF_CloseFont(button_font);
		TTF_CloseFont(counter_font);
		exit(1);
	}

	void DiscordRPC::OnJoin(const char* secret) {
		// Ask to Join is not implemented

		// if(debug) std::cout << "Join: " << secret << std::endl;
	}

	void DiscordRPC::OnJoinRequest(const DiscordUser* user) {
		// Ask to Join is not implemented

		/*
		if(debug) {
			std::cout << "Join request from " << user->username << "#" << user->discriminator
			<< " (" << user->userId << ") " << user->avatar << std::endl;
		}
		*/

		/*
		while(true) {
			std::cout << "Accept? (y/n)" << std::endl;

			std::string line;
			std::getline(std::cin, line);

			if(line == "y") {
				Discord_Respond(user->userId, DISCORD_REPLY_YES);
				break;
			} else if(line == "n") {
				Discord_Respond(user->userId, DISCORD_REPLY_NO);
				break;
			}
		}
		*/
	}

	void DiscordRPC::OnSpectate(const char* secret) {
		if(debug) std::cout << "Spectate: " << secret << std::endl;

		frame = 4;
		renderer.RaiseWindow();

		// TODO: Cancel button
		dialogBox.Set("Connecting to the server...", "");

		if(connected) {
			delete conn;
			connected = false;
		}

		conn = easysock::tcp::connect("themaking.xyz", 34602);
		if(conn == nullptr) {
			dialogBox.Set("Cannot connect to the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
		} else {
			if(conn->write("listen " + std::string(secret) + "|") < 0) {
				dialogBox.Set("Error while communicating with the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
				delete conn;
			} else {
				std::string status = conn->read();
				if(easysock::lastError) {
					dialogBox.Set("Error while communicating with the server (" + std::to_string(easysock::lastError) + " at " + std::to_string(easysock::lastErrorPlace) + ")");
					delete conn;
				} else if(status != "success") {
					if(status == "invalid_token") {
						dialogBox.Set("Invalid token (try observing again)");
					} else {
						dialogBox.Set("Internal server error");
					}
					delete conn;
				} else {
					connected = true;
					spectating = true;
					frame = 1;
				}
			}
		}
	}
#endif
