#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <cstdio>
#include <ctime>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include "../include/files.hpp"
#else
	#include "../include/easysock/tcp.hpp"
	#include "../include/discord.hpp"
#endif
#include "../include/simpleini/SimpleIni.h"
#include "../include/game.hpp"
#include "../include/engine.hpp"

// Game main functions
void MainLoop();
void FrameBegin();
void FrameEnd();
void Frame();

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

// Create engine
Engine engine(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height);

#ifndef __EMSCRIPTEN__
	// Create INI parser
	CSimpleIniA ini(true);
#endif

int main(int argc, char* argv[]) {
	// Needed by rand() function
	srand(time(0));

	#ifndef __EMSCRIPTEN__
		// Parse arguments
		for(int i = 1; i < argc; i++) {
			std::string arg = argv[i];
			if(arg == "--help" || arg == "-h") {
				const char* msg = "  --help -h	Show this message\n"
				                  "  --debug	Enable debugging\n"
				                  "  --demo		Launch game presentation\n"
				                  "  --skip-connect	Skip connecting to the server\n";
				DisplayInfo(title + std::string(" ") + version + std::string("\n") + msg);
				return 0;
			}
			if(arg == "--debug" && !debug) {
				showCounter = true;
				debug = true;
				ReopenConsole();
			}
			if(arg == "--demo" && !demo) {
				demo = true;
			}
			if(arg == "--skip-connect" && !skipconnect) {
				frame = 2;
				skipconnect = true;
			}
		}
	#endif

	// Init engine
	if(!engine.Init()) {
		DisplayError(engine.lastError);
		return 1;
	}

	#ifndef __EMSCRIPTEN__
		// Load INI file
		if(ini.LoadFile("config.ini") == SI_FILE) {
			ini.SetValue("config", "volume", "128");
			ini.SaveFile("config.ini");
		}
		volume = strtol(ini.GetValue("config", "volume", "128"), NULL, 10);

		// Set resource paths
		const char* bgData = "images/bg.png";
		const char* menubgData = "images/menubg.png";
		const char* playerData = "images/player.png";
		const char* buttonFontData = "fonts/DroidSans.ttf";
		const char* counterFontData = "fonts/Visitor2.ttf";
		const char* bgsoundData = "sounds/bgsound.mp3";
	#else
		// Load raw resources
		SDL_RWops* bgData = SDL_RWFromConstMem(bg_raw, bg_raw_size);
		SDL_RWops* menubgData = SDL_RWFromConstMem(menubg_raw, menubg_raw_size);
		SDL_RWops* playerData = SDL_RWFromConstMem(player_raw, player_raw_size);
		SDL_RWops* buttonFontData = SDL_RWFromConstMem(button_font_raw, button_font_raw_size);
		SDL_RWops* counterFontData = SDL_RWFromConstMem(counter_font_raw, counter_font_raw_size);
		SDL_RWops* bgsoundData = SDL_RWFromConstMem(bgsound_raw, bgsound_raw_size);
		if(bgData == NULL || menubgData == NULL || playerData == NULL || buttonFontData == NULL || counterFontData == NULL || bgsoundData == NULL) {
			DisplayError("Can't load required assets (" + std::string(SDL_GetError()) + ")");	
			return 1;
		}
	#endif

	// Load textures
	bg = engine.LoadTexture(bgData);
	menubg = engine.LoadTexture(menubgData);
	player = engine.LoadTexture(playerData);
	if(bg == NULL || menubg == NULL || player == NULL) {
		DisplayError("Can't load required assets (" + std::string(IMG_GetError()) + ")");
		return 1;
	}

	// Load fonts
	buttonFont = engine.LoadFont(buttonFontData, 22);
	optionFont = engine.LoadFont(buttonFontData, 18);
	counterFont = engine.LoadFont(counterFontData, 25);
	if(buttonFont == NULL || optionFont == NULL || counterFont == NULL) {
		DisplayError("Can't load required assets (" + std::string(TTF_GetError()) + ")");
		return 1;
	}

	// Load sounds
	bgsound = engine.LoadSound(bgsoundData);
	if(bgsound == NULL) {
		DisplayError("Can't load required assets (" + std::string(TTF_GetError()) + ")");
		return 1;
	}

	// Setup sounds
	// Channel 0 = background music
	// Channel 1 = other
	Mix_AllocateChannels(2);
	Mix_Volume(0, volume);
	Mix_VolumeChunk(bgsound, 128);

	// Create overlay
	overlay = engine.CreateOverlay(width, height);

	if(showCounter) {
		// Create FPS counter
		counter0 = engine.RenderSolidText(counterFont, "FPS: 0", frame == 1 ? black : dimwhite);
	}

	if(demo) {
		// Set up demo mode
		isPlaying = true;
		frame = 1;
	}

	#ifdef __EMSCRIPTEN__
		// Run main loop
		emscripten_set_main_loop(&MainLoop, 0, 1);
	#else
		// Init easysock (needed on Windows)
		easysock::init();

		// Init Discord Game SDK
		DiscordSDK::Init(411983281886593024);

		// Update Discord Presence
		DiscordSDK::RPC.type = DiscordActivityType_Playing;
		DiscordSDK::RPC.timestamps.start = time(0);
		strcpy(DiscordSDK::RPC.assets.large_image, "square_icon");
		strcpy(DiscordSDK::RPC.assets.large_text, version);
		RandomStr(DiscordSDK::RPC.secrets.match, 32);
		RandomStr(DiscordSDK::RPC.secrets.join, 32);
		RandomStr(DiscordSDK::RPC.secrets.spectate, 32);
		DiscordSDK::UpdateRPC();

		if(!demo && !skipconnect) {
			// Run startup thread
			pthread_create(&thread, NULL, StartupThread, NULL);
		}

		while(true) {
			if(fps > 0) {
				// Get ticks at frame start
				frameTicks = SDL_GetTicks();
			}

			// Run main loop
			MainLoop();
			if(quit) break;

			if(fps > 0) {
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
			engine.SetColor(black);

			// Clear renderer
			engine.Clear();

			// Show render
			engine.Present();
		#endif

		// Destroy resources
		SDL_DestroyTexture(bg);
		SDL_DestroyTexture(menubg);
		SDL_DestroyTexture(player);
		SDL_DestroyTexture(overlay);
		SDL_DestroyTexture(counter0);

		// Destroy fonts
		TTF_CloseFont(buttonFont);
		TTF_CloseFont(optionFont);
		TTF_CloseFont(counterFont);

		// Destroy sounds
		Mix_FreeChunk(bgsound);

		#ifdef __EMSCRIPTEN__
			// Run JavaScript callback
			EM_ASM({
				if(sdlgame_on_exit) sdlgame_on_exit();
			}, NULL);

			// Exit from the loop
			emscripten_cancel_main_loop();
		#else
			// Save config
			ini.SetValue("config", "volume", NumToStr(volume, 0).c_str());
			ini.SaveFile("config.ini");

			// Destroy Discord SDK
			DiscordSDK::Destroy();

			// Quit easysock (needed on Windows)
			easysock::exit();

			// Exit from the loop
			quit = true;
		#endif
		return;
	}

	// Load frame
	if(lastFrame != frame) {
		lastFrame = frame;
		FrameBegin();
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
			// Destroy volume label
			SDL_DestroyTexture(volumeLabel);
			break;
		case 4: // Dialog box
			// Destroy text
			SDL_DestroyTexture(text);
			break;
	}

	// Unload frame (frame change)
	if(lastFrame != frame) {
		FrameEnd();
	}
}

void FrameBegin() {
	if(showCounter) {
		counter0 = engine.RenderSolidText(counterFont, "FPS: " + NumToStr(fpsCount, 0), frame == 1 ? black : dimwhite);
	}

	switch(frame) {
		case 1: // Game
			#ifndef __EMSCRIPTEN__
				// Update Discord Presence
				strcpy(DiscordSDK::RPC.state, "In Game");
				strcpy(DiscordSDK::RPC.details, (spectating ? "Spectating someone" : (demo ? "Watching demo" : "Stage 1")));
				DiscordSDK::UpdateRPC();
			#endif

			// Play background music
			if(!Mix_Playing(0)) {
				Mix_PlayChannel(0, bgsound, -1);
			} else {
				Mix_Resume(0);
			}
			break;
		case 2: // Main menu
			#ifndef __EMSCRIPTEN__
				// Update Discord Presence
				strcpy(DiscordSDK::RPC.state, "In Main Menu");
				strcpy(DiscordSDK::RPC.details, "Wasting some time");
				DiscordSDK::UpdateRPC();
			#endif
			break;
		case 3: // Options
			#ifndef __EMSCRIPTEN__
				// Update Discord Presence
				strcpy(DiscordSDK::RPC.state, "In Main Menu");
				strcpy(DiscordSDK::RPC.details, "Changing some options");
				DiscordSDK::UpdateRPC();
			#endif

			// Create trackbar
			trackbar = engine.CreateTexture(9, 25, SDL_TEXTUREACCESS_TARGET);
			engine.SetTarget(trackbar);
			engine.SetColor(white);
			engine.Clear();
			engine.SetColor(black);
			engine.DrawLine(2, 8, 5, 1);
			engine.DrawLine(2, 12, 5, 1);
			engine.DrawLine(2, 16, 5, 1);
			engine.SetColor(white);
			engine.SetTarget(NULL);
			break;
		case 4: // Dialog box
			#ifndef __EMSCRIPTEN__
				// Update Discord Presence
				strcpy(DiscordSDK::RPC.state, "In Main Menu");
				strcpy(DiscordSDK::RPC.details, "Wasting some time");
				DiscordSDK::UpdateRPC();
			#endif
			break;
	}
}

void FrameEnd() {
	switch(lastFrame) {
		case 1: // Game
			Mix_Pause(0);
			break;
		case 2: // Main menu
			//
			break;
		case 3: // Options
			// Destroy trackbar
			SDL_DestroyTexture(trackbar);
			break;
		case 4: // Dialog box
			//
			break;
	}
}

void Frame() {
	if(showCounter) {
		// FPS counting
		fpsFrames++;
		if(fpsFrameTicks + 1000 < SDL_GetTicks()) {
			fpsCount = fpsFrames;
			fpsFrames = 0;
			counter0 = engine.RenderSolidText(counterFont, "FPS: " + NumToStr(fpsCount, 0), frame == 1 ? black : dimwhite);
			fpsFrameTicks = SDL_GetTicks();
		}
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
		DiscordSDK::RunTasks();
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
		counter0 = engine.RenderSolidText(counterFont, "FPS: " + NumToStr(fpsCount, 0), frame == 1 ? black : dimwhite);
	}

	// Show main menu, back to game or exit
	if(key[SDL_SCANCODE_ESCAPE] && !escKey) {
		escKey = true;
		if(frame != 4 || !dialogBox.buttonText.empty()) {
			frame = (demo ? 0 : (frame != 2 ? 2 : (isPlaying ? 1 : 0)));
		}
		return;
	}

	switch(frame) {
		case 1: // Game
			#ifndef __EMSCRIPTEN__
			if(!spectating) {
			#endif
				if(!demo) {
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
					if(posY < height - sizeY) {
						if(jumpState != 3 && velocityY > 200) {
							jumpState = 3;
						}
						velocityY += gravity * delta;
					} else if(jumpState > 0) {
						jumpState = 0;
						velocityY = 0;
					}
				} else {
					if(demoDirection) { // Left
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
				}

				// Move character if it's below bottom barrier of the window
				if(posY > height - sizeY) {
					posY = height - sizeY;
				}

				// Go to next frame OR stop player on edge of window
				if(gameFrame + 1 <= GAME_FRAMES) {
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

				/*if(CheckCollision()) {
					if(posY + sizeY > rect.y && jumpState == 3) {
						posY = rect.y - sizeY;
					} else if(posY < rect.y + rect.h && (jumpState == 1 || jumpState == 2)) {
						posY = rect.y + rect.h;
					} else if(posX + sizeX > rect.x && key[SDL_SCANCODE_RIGHT]) {
						posX = rect.x - sizeX;
					} else if(posX < rect.x + rect.w && key[SDL_SCANCODE_LEFT]) {
						posX = rect.x + rect.w;
					}
				}*/

				if(!demo) {
					#ifndef __EMSCRIPTEN__
						// Send player position to the server
						if(connected && (posX != tmpX || posY != tmpY)) {
							tmpX = posX;
							tmpY = posY;
							if(conn->write("send " + Serialize(gameFrame, posX, posY) + "|") < 0) {
								frame = 4;
								dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
								pthread_create(&thread, NULL, StartupThread, NULL);
								return;
							}
						}
					#endif

					// On game frame change
					if(gameFrame != lastGameFrame) {
						lastGameFrame = gameFrame;

						#ifndef __EMSCRIPTEN__
							// Update Discord Presence
							strcpy(DiscordSDK::RPC.details, ("Stage " + NumToStr(gameFrame, 0)).c_str());
							DiscordSDK::UpdateRPC();
						#endif
					}
				}
			#ifndef __EMSCRIPTEN__
			} else if(connected) {
				// Get player position from the server
				std::string status = conn->read();
				if(easysock::lastError) {
					dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
					spectating = false;
					pthread_create(&thread, NULL, StartupThread, NULL);
					return;
				} else {
					Unserialize(status, gameFrame, posX, posY);
				}
			}
			#endif

			// Reload counter
			if(showCounter) {
				counter1 = engine.RenderSolidText(counterFont, "X: " + NumToStr(posX), black);
				counter2 = engine.RenderSolidText(counterFont, "Y: " + NumToStr(posY), black);
				counter3 = engine.RenderSolidText(counterFont, "Frame: " + NumToStr(gameFrame, 0) + "/" + NumToStr(GAME_FRAMES, 0), black);
				counter4 = engine.RenderSolidText(counterFont, "Jump state: " + NumToStr(jumpState, 0), black);
				counter5 = engine.RenderSolidText(counterFont, "Velocity: " + NumToStr(velocityY), black);
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
					if(!isPlaying) isPlaying = true;
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
					if(!isPlaying) isPlaying = true;
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
			// Dragging trackbar
			if(mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				if(mouseX != lastX || mouseY != lastY) {
					if(!mouseLock) {
						if(mouseX >= 126 + volume && mouseX <= 135 + volume && mouseY >= 30 && mouseY <= 55) {
							mouseLock = true;
						} else if(mouseX >= 130 && mouseX <= 258 && mouseY >= 40 && mouseY <= 45) {
							volume = mouseX - 130;
						}
					} else {
						if(mouseX < 130) {
							volume = 0;
						} else if(mouseX > 258) {
							volume = 128;
						} else {
							volume = mouseX - 130;
						}
					}
					Mix_Volume(0, volume);
					lastX = mouseX;
					lastY = mouseY;
				}
			}

			// Update volume label
			volumeLabel = engine.RenderText(optionFont, "Volume (" + NumToStr(volume / 128.0 * 100, 0) + ")", white);
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
			text = engine.RenderText(buttonFont, dialogBox.text, white);
			break;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Clear renderer
	engine.Clear();

	switch(frame) {
		case 1: // Game
			// Render background (flip horizontally if gameFrame is even)
			engine.Draw(bg, NULL, NULL, 0, NULL, gameFrame % 2 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);

			// Render game frames
			switch(gameFrame) {
				case 1:
					// Render triangles
					engine.DrawTriangle(width - 110, height / 2 - 100, black, 100, TRIANGLE_RIGHT);
					engine.DrawTriangle(width - 350, height / 2 - 100, black, 100, TRIANGLE_UP);
					engine.DrawTriangle(250, height / 2 - 100, black, 100, TRIANGLE_DOWN);
					engine.DrawTriangle(10, height / 2 - 100, black, 100, TRIANGLE_LEFT);
					break;
				case 2:
					// Render left and right triangle
					engine.DrawTriangle(10, height / 2 - 100, black, 100, TRIANGLE_LEFT);
					engine.DrawTriangle(width - 110, height / 2 - 100, black, 100, TRIANGLE_RIGHT);
					break;
				case 3:
					// Render left triangle
					engine.DrawTriangle(10, height / 2 - 100, black, 100, TRIANGLE_LEFT);
					break;
			}

			for(uint8_t i = 0; i < collisionCounts[gameFrame - 1]; i++) {
				SDL_RenderFillRect(engine.r, &collisions[gameFrame - 1][i]);
			}

			// Set player size and position
			rect.x = posX;
			rect.y = posY;
			rect.w = sizeX;
			rect.h = sizeY;

			// Render player
			engine.Draw(player, NULL, &rect, 0, NULL, flip);

			if(showCounter) {
				// Loop through counter lines
				for(int i = 1; i <= 5; i++) {
					// Get counter line by index
					SDL_Texture* counter = (i == 1 ? counter1 : i == 2 ? counter2 : i == 3 ? counter3 : i == 4 ? counter4 : i == 5 ? counter5 : NULL);

					// Display counter line
					SDL_QueryTexture(counter, NULL, NULL, &rect.w, &rect.h);
					rect.x = 10;
					rect.y = 4 + (18 * i);
					engine.Draw(counter, NULL, &rect);
				}
			}
			break;
		case 2: // Main menu
			// Render background
			engine.Draw(menubg, NULL, NULL);

			// Render background overlay
			engine.Draw(overlay, NULL, NULL);

			// Set button size and position
			rect.x = 300;
			rect.y = 200;
			rect.w = 200;
			rect.h = 50;

			// Render button
			engine.DrawButton(buttonFont, isPlaying ? "Back to game" : "Play", rect, white, green, option == 1 ? red : green);

			// Set button size and position
			rect.x = 300;
			rect.y = 260;
			rect.w = 200;
			rect.h = 50;

			// Render button
			engine.DrawButton(buttonFont, "Options", rect, white, green, option == 2 ? red : green);

			// Set button size and position
			rect.x = 300;
			rect.y = 320;
			rect.w = 200;
			rect.h = 50;

			// Render button
			engine.DrawButton(buttonFont, "Exit", rect, white, green, option == 3 ? red : green);
			break;
		case 3: // Options
			// Render background
			engine.Draw(menubg, NULL, NULL);

			// Render background overlay
			engine.Draw(overlay, NULL, NULL);

			// Display volume label
			SDL_QueryTexture(volumeLabel, NULL, NULL, &rect.w, &rect.h);
			rect.x = 10;
			rect.y = 30;
			engine.Draw(volumeLabel, NULL, &rect);

			// Display volume trackbar
			rect.x = 130;
			rect.y = 40;
			rect.w = 128;
			rect.h = 5;
			engine.SetColor(dimwhite);
			SDL_RenderFillRect(engine.r, &rect);
			engine.SetColor(white);

			rect.x = 126 + volume;
			rect.y = 30;
			rect.w = 9;
			rect.h = 25;
			engine.Draw(trackbar, NULL, &rect);
			break;
		case 4: // Dialog box
			// Render background
			engine.Draw(menubg, NULL, NULL);

			// Render background overlay
			engine.Draw(overlay, NULL, NULL);

			// Display text
			SDL_QueryTexture(text, NULL, NULL, &rect.w, &rect.h);
			rect.x = width / 2 - rect.w / 2;
			rect.y = 260;
			engine.Draw(text, NULL, &rect);

			// Set button size and position
			rect.x = 300;
			rect.y = 300;
			rect.w = 200;
			rect.h = 50;

			// If dialog box buttons is shown
			if(!dialogBox.buttonText.empty()) {
				// Render button
				engine.DrawButton(buttonFont, dialogBox.buttonText, rect, white, green, red);
			}
			break;
	}

	if(showCounter) {
		// Display FPS counter
		SDL_QueryTexture(counter0, NULL, NULL, &rect.w, &rect.h);
		rect.x = 10;
		rect.y = 4;
		engine.Draw(counter0, NULL, &rect);
	}

	// Show render
	engine.Present();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __EMSCRIPTEN__
	void* StartupThread(void*) {
		// TODO: Skip button
		dialogBox.Set("Connecting to the server...", "");

		if(connected) {
			delete conn;
			connected = false;
		}

		conn = easysock::tcp::connect("themaking.tk", 34602);
		if(conn == nullptr) {
			dialogBox.Set("Cannot connect to the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
		} else {
			if(conn->write("connect " + std::string(DiscordSDK::RPC.secrets.spectate) + "|") < 0) {
				dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
				delete conn;
			} else {
				std::string status = conn->read();
				if(easysock::lastError) {
					dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
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

	void DiscordSDK::OnError(void* data, enum EDiscordResult result) {
		Log("[Discord] Error: " + std::string(DiscordSDK::GetResultStr(result)));
	}

	void DiscordSDK::OnRpcUpdate(void* data, enum EDiscordResult result) {
		Log("[Discord] RPC Update (Result: " + std::string(DiscordSDK::GetResultStr(result)) + ")");
	}

	void DiscordSDK::OnUserUpdate(void* data) {
		struct DiscordSDK::Application* app = (struct DiscordSDK::Application*)data;
		struct DiscordUser user;
		app->users->get_current_user(app->users, &user);
		Log("[Discord] Current user: " + std::string(user.username) + "#" + std::string(user.discriminator) + " (" + std::to_string(user.id) + ")");
	}

	void DiscordSDK::OnJoinRequest(void* data, struct DiscordUser* user) {
		Log("[Discord] Join request from " + std::string(user->username) + "#" + std::string(user->discriminator) + " (" + std::to_string(user->id) + ")");

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

	void DiscordSDK::OnJoin(void* data, const char* secret) {
		Log("[Discord] Join " + std::string(secret));
	}

	void DiscordSDK::OnSpectate(void* data, const char* secret) {
		Log("[Discord] Spectate " + std::string(secret));

		frame = 4;
		engine.RaiseWindow();

		// TODO: Cancel button
		dialogBox.Set("Connecting to the server...", "");

		if(connected) {
			delete conn;
			connected = false;
		}

		conn = easysock::tcp::connect("themaking.xyz", 34602);
		if(conn == nullptr) {
			dialogBox.Set("Cannot connect to the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
		} else {
			if(conn->write("listen " + std::string(secret) + "|") < 0) {
				dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
				delete conn;
			} else {
				std::string status = conn->read();
				if(easysock::lastError) {
					dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
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

	void DiscordSDK::OnInvite(void* data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity) {
		Log("[Discord] Invite type " + std::to_string(type) + " to '" + std::string(activity->name) + "' from " + std::string(user->username) + "#" + std::string(user->discriminator) + " (" + std::to_string(user->id) + ")");
	}
#endif
