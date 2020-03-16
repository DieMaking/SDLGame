#define SDL_MAIN_HANDLED

#include <ctime>
#include <cstdio>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#ifndef __EMSCRIPTEN__
	#include <SDL2/SDL_mixer.h>
	#include "../include/easysock/tcp.hpp"
	#include "../include/simpleini/SimpleIni.h"
	#ifndef NDISCORD
		#include "../include/discord.hpp"
	#endif
#else
	#include <emscripten.h>
	#include "../include/files.hpp"
#endif
#include "../include/game.hpp"
#include "../include/engine.hpp"

// Game main functions
void MainLoop();
void FrameBegin();
void FrameEnd();
void Frame();

// Create engine
Engine engine(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height);

#ifndef __EMSCRIPTEN__
	// Create INI parser
	CSimpleIniA ini(true);

	#ifndef NDISCORD
		// Create Discord SDK
		DiscordSDK discord(411983281886593024);
	#endif
#else
	// Functions for getting/setting config values in browser localStorage
	EM_JS(char*, sdlgame_get_cfg_val, (const char* name), {
		var tmp = UTF8ToString(name);
		if(localStorage[tmp]) {
			var value = localStorage[tmp];
			var len = lengthBytesUTF8(value) + 1;
			var heap = _malloc(len);
			stringToUTF8(value, heap, len);
			return heap;
		}
		return null;
	});
	EM_JS(void, sdlgame_set_cfg_val, (const char* name, const char* value), {
		var tmp = UTF8ToString(name);
		if(tmp) {
			localStorage[tmp] = UTF8ToString(value);
		}
	});
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
				isPlaying = true;
				frame = 1;
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
		if(!demo) {
			// Load INI file
			if(ini.LoadFile("config.ini") == SI_FILE) {
				ini.SetValue("config", "volume", "100");
				ini.SaveFile("config.ini");
			}
			// Get volume
			volume = strtoul(ini.GetValue("config", "volume", "100"), NULL, 10);
			if(volume > 100) volume = 100;
		}

		// Set resource paths
		const char* bgData = "images/bg.png";
		const char* menubgData = "images/menubg.png";
		const char* playerData = "images/player.png";
		const char* buttonFontData = "fonts/DroidSans.ttf";
		const char* optionFontData = "fonts/DroidSans.ttf";
		const char* counterFontData = "fonts/Visitor2.ttf";
	#else
		if(!demo) {
			// Get volume
			char* val = sdlgame_get_cfg_val("volume");
			if(val != NULL) {
				volume = strtoul(val, NULL, 10);
				free(val);
				if(volume > 100) volume = 100;
			}
		}

		// Load raw resources
		SDL_RWops* bgData = SDL_RWFromConstMem(bg_raw, bg_raw_size);
		SDL_RWops* menubgData = SDL_RWFromConstMem(menubg_raw, menubg_raw_size);
		SDL_RWops* playerData = SDL_RWFromConstMem(player_raw, player_raw_size);
		SDL_RWops* buttonFontData = SDL_RWFromConstMem(button_font_raw, button_font_raw_size);
		SDL_RWops* optionFontData = SDL_RWFromConstMem(button_font_raw, button_font_raw_size);
		SDL_RWops* counterFontData = SDL_RWFromConstMem(counter_font_raw, counter_font_raw_size);
		if(bgData == NULL || menubgData == NULL || playerData == NULL || buttonFontData == NULL || optionFontData == NULL || counterFontData == NULL) {
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
	optionFont = engine.LoadFont(optionFontData, 18);
	counterFont = engine.LoadFont(counterFontData, 25);
	if(buttonFont == NULL || optionFont == NULL || counterFont == NULL) {
		DisplayError("Can't load required assets (" + std::string(TTF_GetError()) + ")");
		return 1;
	}

	#ifndef __EMSCRIPTEN__
		// Load sounds
		bgsound = engine.LoadSound("sounds/bgsound.mp3");
		if(bgsound == NULL) {
			DisplayError("Can't load required assets (" + std::string(Mix_GetError()) + ")");
			return 1;
		}

		// Setup sounds
		// Channel 0 = background music
		// Channel 1 = other
		Mix_AllocateChannels(2);
		Mix_Volume(0, volume / 100.0 * MIX_MAX_VOLUME);
	#endif

	// Create overlay
	overlay = engine.CreateOverlay(width, height);

	// Create game frames cache
	render1 = engine.CreateTexture(width, height, SDL_TEXTUREACCESS_TARGET);
	render2 = engine.CreateTexture(width, height, SDL_TEXTUREACCESS_TARGET);

	if(showCounter) {
		// Create FPS counter
		counter0 = engine.RenderSolidText(counterFont, "FPS: 0", frame == 1 ? black : dimwhite);
	}

	#ifndef __EMSCRIPTEN__
		// Init easysock (needed on Windows)
		easysock::init();

		#ifndef NDISCORD
			// Discord Game SDK callbacks
			discord.OnError = [](void* data, enum EDiscordResult result) {
				Log("[Discord] Error: " + std::string(discord.GetResultStr(result)));
			};
			discord.OnRpcUpdate = [](void* data, enum EDiscordResult result) {
				Log("[Discord] RPC Update (Result: " + std::string(discord.GetResultStr(result)) + ")");
			};
			discord.OnUserUpdate = [](void* data) {
				DiscordSDK* app = (DiscordSDK*)data;
				struct DiscordUser user;
				app->users->get_current_user(app->users, &user);
				Log("[Discord] Current user: " + std::string(user.username) + "#" + std::string(user.discriminator) + " (" + std::to_string(user.id) + ")");
			};
			discord.OnJoinRequest = [](void* data, struct DiscordUser* user) {
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
			};
			discord.OnJoin = [](void* data, const char* secret) {
				Log("[Discord] Join " + std::string(secret));
			};
			discord.OnSpectate = [](void* data, const char* secret) {
				Log("[Discord] Spectate " + std::string(secret));

				/*
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
				*/
			};
			discord.OnInvite = [](void* data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity) {
				Log("[Discord] Invite type " + std::to_string(type) + " to '" + std::string(activity->name) + "' from " + std::string(user->username) + "#" + std::string(user->discriminator) + " (" + std::to_string(user->id) + ")");
			};

			// Init Discord Game SDK
			discord.Init();

			// Update Discord Presence
			discord.rpc.type = DiscordActivityType_Playing;
			discord.rpc.timestamps.start = time(0);
			strcpy(discord.rpc.assets.large_image, "square_icon");
			strcpy(discord.rpc.assets.large_text, version);
			RandomStr(discord.rpc.secrets.match, 32);
			RandomStr(discord.rpc.secrets.join, 32);
			RandomStr(discord.rpc.secrets.spectate, 32);
			discord.UpdateRPC();

			if(!demo && !skipconnect) {
				// Run startup thread
				pthread_create(&thread, NULL, StartupThread, NULL);
			}
		#endif

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
	#else
		// Run main loop
		emscripten_set_main_loop(&MainLoop, 0, 1);
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
		SDL_DestroyTexture(render1);
		SDL_DestroyTexture(render2);
		SDL_DestroyTexture(counter0);

		// Destroy fonts
		TTF_CloseFont(buttonFont);
		TTF_CloseFont(optionFont);
		TTF_CloseFont(counterFont);

		#ifndef __EMSCRIPTEN__
			// Destroy sounds
			Mix_FreeChunk(bgsound);

			// Quit easysock (needed on Windows)
			easysock::exit();

			// Exit from the loop
			quit = true;
		#else
			// Close game window
			EM_ASM({
				if(sdlgame_on_exit) sdlgame_on_exit();
			}, NULL);

			// Exit from the loop
			emscripten_cancel_main_loop();
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
				#ifndef NDISCORD
					// Update Discord Presence
					strcpy(discord.rpc.state, "In Game");
					strcpy(discord.rpc.details, (spectating ? "Spectating someone" : (demo ? "Watching demo" : "Stage 1")));
					discord.UpdateRPC();
				#endif

				// Play background music
				if(!Mix_Playing(0)) {
					Mix_PlayChannel(0, bgsound, -1);
				} else {
					Mix_Resume(0);
				}
			#else
				// Play background music
				EM_ASM({
					if(sdlgame_bgsound_play) sdlgame_bgsound_play(true);
				}, NULL);
			#endif
			break;
		case 2: // Main menu
			#ifndef __EMSCRIPTEN__
				#ifndef NDISCORD
					// Update Discord Presence
					strcpy(discord.rpc.state, "In Main Menu");
					strcpy(discord.rpc.details, "Wasting some time");
					discord.UpdateRPC();
				#endif
			#endif
			break;
		case 3: // Options
			#ifndef __EMSCRIPTEN__
				#ifndef NDISCORD
					// Update Discord Presence
					strcpy(discord.rpc.state, "In Main Menu");
					strcpy(discord.rpc.details, "Changing some options");
					discord.UpdateRPC();
				#endif
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

			// Reset mouse lock
			mouseLock = false;
			break;
		case 4: // Dialog box
			#ifndef __EMSCRIPTEN__
				#ifndef NDISCORD
					// Update Discord Presence
					strcpy(discord.rpc.state, "In Main Menu");
					strcpy(discord.rpc.details, "Wasting some time");
					discord.UpdateRPC();
				#endif
			#endif
			break;
	}
}

void FrameEnd() {
	switch(lastFrame) {
		case 1: // Game
			// Pause background music
			#ifndef __EMSCRIPTEN__
				Mix_Pause(0);
			#else
				EM_ASM({
					if(sdlgame_bgsound_play) sdlgame_bgsound_play(false);
				}, NULL);
			#endif
			break;
		case 2: // Main menu
			//
			break;
		case 3: // Options
			// Destroy trackbar
			SDL_DestroyTexture(trackbar);

			// Save config
			#ifndef __EMSCRIPTEN__
				ini.SetValue("config", "volume", NumToStr(volume, 0).c_str());
				ini.SaveFile("config.ini");
			#else
				sdlgame_set_cfg_val("volume", NumToStr(volume, 0).c_str());
			#endif
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
		#ifndef NDISCORD
			// Run Discord Presence tasks
			discord.RunTasks();
		#endif
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
			if(!spectating) {
				// If game frame is not changing
				if(gameFrameChange == 0) {
					// If not in demo mode
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

						// Must be used to prevent bugs, this will be shooting in the futsure
						if((mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) && !mouseLock) {
							mouseLock = true;
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

					// Go to next frame OR stop player on edge of window
					if(gameFrame + 1 <= GAME_FRAMES) {
						if(posX >= width - sizeX / 2) {
							if(gameFrame == lastGameFrame) {
								gameFrameChange = 1;
							} else {
								posX = width - sizeX / 2 - 1;
							}
						}
					} else if(posX > width - sizeX) {
						posX = width - sizeX;
						if(demo) demoDirection = true;
					}

					// Go to previous frame OR stop player on edge of window
					if(gameFrame - 1 > 0) {
						if(posX <= sizeX / 2 - sizeX) {
							if(gameFrame == lastGameFrame) {
								gameFrameChange = -1;
							} else {
								posX = sizeX / 2 - sizeX + 1;
							}
						}
					} else if(posX < 1) {
						posX = 1;
						if(demo) demoDirection = false;
					}
				}

				// Move character if it's below bottom barrier of the window
				if(posY > height - sizeY) {
					posY = height - sizeY;
				}

				/* TODO: Make this working
				if(CheckCollision()) {
					if(posY + sizeY > rect.y && jumpState == 3) {
						posY = rect.y - sizeY;
					} else if(posY < rect.y + rect.h && (jumpState == 1 || jumpState == 2)) {
						posY = rect.y + rect.h;
					} else if(posX + sizeX > rect.x && key[SDL_SCANCODE_RIGHT]) {
						posX = rect.x - sizeX;
					} else if(posX < rect.x + rect.w && key[SDL_SCANCODE_LEFT]) {
						posX = rect.x + rect.w;
					}
				}
				*/

				// On game frame change
				if(gameFrameChange == 0 && gameFrame != lastGameFrame) {
					lastGameFrame = gameFrame;

					#ifndef __EMSCRIPTEN__
						#ifndef NDISCORD
							// Update Discord Presence
							strcpy(discord.rpc.details, ("Stage " + NumToStr(gameFrame, 0)).c_str());
							discord.UpdateRPC();
						#endif
					#endif
				}

				// Send player position to the server
				if(!demo && connected && (posX != tmpX || posY != tmpY)) {
					tmpX = posX;
					tmpY = posY;
					#ifndef NDISCORD
						#ifndef __EMSCRIPTEN__
							if(conn->write("send " + Serialize(lastGameFrame, posX, posY) + "|") < 0) {
								frame = 4;
								dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
								pthread_create(&thread, NULL, StartupThread, NULL);
								return;
							}
						#endif
					#endif
				}
			} else if(connected) {
				// Get player position from the server
				#ifndef NDISCORD
					#ifndef __EMSCRIPTEN__
						std::string status = conn->read();
						if(easysock::lastError) {
							dialogBox.Set("Error while communicating with the server (" + NumToStr(easysock::lastError, 0) + " at " + NumToStr(easysock::lastErrorPlace, 0) + ")");
							spectating = false;
							pthread_create(&thread, NULL, StartupThread, NULL);
							return;
						} else {
							Unserialize(status, gameFrame, posX, posY);
						}
					#endif
				#endif
			}

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
						} else if(mouseX >= 130 && mouseX <= 230 && mouseY >= 40 && mouseY <= 45) {
							volume = mouseX - 130;
						}
					} else {
						if(mouseX < 130) {
							volume = 0;
						} else if(mouseX > 230) {
							volume = 100;
						} else {
							volume = mouseX - 130;
						}
					}
					#ifndef __EMSCRIPTEN__
						Mix_Volume(0, volume / 100.0 * MIX_MAX_VOLUME);
					#else
						EM_ASM({
							if(sdlgame_volume_change) sdlgame_volume_change($0);
						}, volume / 100.0);
					#endif
					lastX = mouseX;
					lastY = mouseY;
				}
			}

			// Update volume label
			volumeLabel = engine.RenderText(optionFont, "Volume (" + NumToStr(volume, 0) + ")", white);
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
			if(gameFrame == lastGameFrame) {
				// Executed when current game frame is about to render
				if(gameFrameChange != 0) {
					// Render current game frame to the cache
					engine.SetTarget(render1);
					engine.Clear();
				}

				do {
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

					// Executed when new game frame is about to render
					if(gameFrameChange != 0 && gameFrame == lastGameFrame) {
						// Render new game frame to the cache
						gameFrame += gameFrameChange;
						posX += (gameFrameChange < 0 ? width : -width);
						engine.SetTarget(render2);
						engine.Clear();
					} else {
						break;
					}
				} while(1);

				// Executed when game frames has been rendered
				if(gameFrameChange != 0) {
					engine.SetTarget(NULL);
					renderPos = 0;
				}
			}
			if(gameFrameChange != 0) {
				// Animate game frame change
				rect.x = renderPos;
				rect.y = 0;
				rect.w = width;
				rect.h = height;
				engine.Draw(render1, NULL, &rect);

				rect.x = renderPos + (gameFrameChange < 0 ? -width : width);
				rect.y = 0;
				rect.w = width;
				rect.h = height;
				engine.Draw(render2, NULL, &rect);

				renderPos -= gameFrameChange * (width / 20);
				if(renderPos <= -width || renderPos >= width) {
					gameFrameChange = 0;
				}
			}

			if(showCounter) {
				// Loop through counter lines
				for(int i = 1; i <= 5; i++) {
					// Get counter line by index
					SDL_Texture* counter = (i == 1 ? counter1 : i == 2 ? counter2 : i == 3 ? counter3 : i == 4 ? counter4 : i == 5 ? counter5 : NULL);

					// Display counter line
					engine.QueryTexture(counter, &rect);
					rect.x = 10;
					rect.y = 4 + (18 * i);
					engine.Draw(counter, NULL, &rect);
				}
			}
			break;
		case 2: // Main menu
			// Render background
			engine.Draw(menubg);

			// Render background overlay
			engine.Draw(overlay);

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
			engine.Draw(menubg);

			// Render background overlay
			engine.Draw(overlay);

			// Display volume label
			engine.QueryTexture(volumeLabel, &rect);
			rect.x = 10;
			rect.y = 30;
			engine.Draw(volumeLabel, NULL, &rect);

			// Display volume trackbar
			rect.x = 130;
			rect.y = 40;
			rect.w = 100;
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
			engine.Draw(menubg);

			// Render background overlay
			engine.Draw(overlay);

			// Display text
			engine.QueryTexture(text, &rect);
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
		engine.QueryTexture(counter0, &rect);
		rect.x = 10;
		rect.y = 4;
		engine.Draw(counter0, NULL, &rect);
	}

	// Show render
	engine.Present();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __EMSCRIPTEN__
	#ifndef NDISCORD
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
				if(conn->write("connect " + std::string(discord.rpc.secrets.spectate) + "|") < 0) {
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
	#endif
#endif
