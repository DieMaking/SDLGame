# SDLGame
SDLGame is a simple platform game written in C++ using SDL2

## Compilation
### Linux
* **Set up a build environment**
	* Install needed software
		* Debian
		```
		# apt install build-essential git unzip libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev libsdl2-mixer-2.0-0 libsdl2-mixer-dev libsdl2-ttf-2.0-0 libsdl2-ttf-dev
		```
		* Arch
		```
		# pacman -S base-devel git unzip sdl2 sdl2_image sdl2_mixer sdl2_ttf
		```
	* Download discord-rpc library from [here](https://themaking.xyz/r/discord-rpc-linux-latest)
	* Copy contents of the `linux-dynamic` folder from discord-rpc to `/usr/`
* **Get the required files**
	* Clone this repository OR Download ZIP of this repository
	```
	$ git clone https://github.com/DieMaking/SDLGame.git
	OR
	$ wget https://github.com/DieMaking/SDLGame/archive/master.zip && unzip master.zip
	```
* **Compile and run**
	* If you cloned this repository, `WORK_DIR` is `SDLGame`, if you downloaded ZIP of this repository, `WORD_DIR` is `SDLGame-master`
	```
	$ cd WORD_DIR
	$ make BUILD=release
	$ make run
	```

### Windows
* **Set up a build environment**
	* Download MinGW from [here](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/i686-8.1.0-release-posix-dwarf-rt_v6-rev0.7z/download) and unzip it to `C:\MinGW`
	* Add `C:\MinGW` to your PATH ([tutorial](https://www.howtogeek.com/118594/how-to-edit-your-system-path-for-easy-command-line-access/))
	* Download SDL2 archives:
		* [SDL2](https://themaking.xyz/r/sdl-latest)
		* [SDL2_image](https://themaking.xyz/r/sdl-image-latest)
		* [SDL2_mixer](https://themaking.xyz/r/sdl-mixer-latest)
		* [SDL2_ttf](https://themaking.xyz/r/sdl-ttf-latest)
	* Unpack the `i686-w64-mingw32` folder content from each archive to `C:\MinGW`
	* Download discord-rpc archive from [here](https://themaking.xyz/r/discord-rpc-win-latest)
	* Unpack the `win32-dynamic` folder content to `C:\MinGW`
	* Download Git from [here](https://git-scm.com/download/win) and install it
	* Right-click on desktop and click "Git Bash Here"
* **Get the required files**
	* Clone this repository OR Download ZIP of this repository
	```
	$ git clone https://github.com/DieMaking/SDLGame.git
	OR
	$ wget https://github.com/DieMaking/SDLGame/archive/master.zip && unzip master.zip
	```
* **Compile and run**
	* If you cloned this repository, `WORK_DIR` is `SDLGame`, if you downloaded ZIP of this repository, `WORD_DIR` is `SDLGame-master`
	```
	$ cd WORD_DIR
	$ make BUILD=release
	$ make run
	```

You can also use [MSYS2](https://www.msys2.org/) for compiling SDLGame on Windows
