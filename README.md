# SDLGame
SDLGame is a simple platform game written in C++ using SDL2

## Compilation
### Linux
* **Set up a build environment**
	* Install needed software
		* Debian
		```
		# apt install build-essential git libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev libsdl2-mixer-2.0-0 libsdl2-mixer-dev libsdl2-ttf-2.0-0 libsdl2-ttf-dev
		```
		* Arch
		```
		# pacman -S base-devel git sdl2 sdl2_image sdl2_mixer sdl2_ttf
		```
	* Download Discord Game SDK from [here](https://dl-game-sdk.discordapp.net/latest/discord_game_sdk.zip)
	* Extract the following files from the archive:
		* `lib/x86_64/discord_game_sdk.so` to `/usr/lib/`
		* `c/discord_game_sdk.h` to `/usr/include/`
* **Get required files**
	* Clone this repository
	```
	$ git clone --recursive https://github.com/DieMaking/SDLGame.git
	```
* **Compile and run**
	```
	$ cd SDLGame
	$ make BUILD=release
	$ make run
	```

### Windows
* **Set up a build environment**
	* Download MinGW from [here](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/i686-8.1.0-release-posix-dwarf-rt_v6-rev0.7z/download) and unzip it to `C:\MinGW`
	* Add `C:\MinGW` to your PATH ([tutorial](https://www.howtogeek.com/118594/how-to-edit-your-system-path-for-easy-command-line-access/))
	* Download SDL2 archives:
		[SDL2](https://www.libsdl.org/release/SDL2-devel-2.0.10-mingw.tar.gz),
		[SDL2_image](https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-2.0.5-mingw.tar.gz),
		[SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-mingw.tar.gz),
		[SDL2_ttf](https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.15-mingw.tar.gz)
	* Unpack the `i686-w64-mingw32` folder content from each archive to `C:\MinGW`
	* Download Discord Game SDK from [here](https://dl-game-sdk.discordapp.net/latest/discord_game_sdk.zip)
	* Extract the following files from the archive:
		* `lib\x86\discord_game_sdk.dll` to `C:\MinGW\bin\`
		* `lib\x86\discord_game_sdk.dll.lib` to `C:\MinGW\lib\`
		* `c\discord_game_sdk.h` to `C:\MinGW\include\`
	* Download Git from [here](https://git-scm.com/download/win) and install it
	* Right-click on the desktop and click "Git Bash Here"
* **Get required files**
	* Clone this repository
	```
	$ git clone --recursive https://github.com/DieMaking/SDLGame.git
	```
* **Compile and run**
	```
	$ cd SDLGame
	$ make BUILD=release
	$ make run
	```

You can also use [MSYS2](https://www.msys2.org/) for compiling SDLGame on Windows
