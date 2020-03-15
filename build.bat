@echo off
cd /d "%~dp0"
:: TODO: Merge with Makefile
if "%~1" == "resources" (
    echo Building resources...
    cd SDLGame_Windows
    bin2c -o ..\include\files.hpp -n button_font_raw fonts\DroidSans.ttf -n counter_font_raw fonts\Visitor2.ttf -n bg_raw images\bg.png -n menubg_raw images\menubg.png -n player_raw images\player.png
    cd ..
    if errorlevel 9009 (
        echo You don't have bin2c installed, download it from https://sourceforge.net/projects/bin2c/
        exit /b 1
    )
) else (
    where em++ >nul 2>&1
    if errorlevel 1 (
        echo Emscripten is not in the PATH, trying to add it...
        call "C:\emsdk\emsdk_env.bat" >nul 2>&1
        if errorlevel 1 (
            echo Cannot find emscripten in C:\emsdk
            exit /b 1
        )
    )
    echo Building...
    mkdir SDLGame_Web >nul 2>&1
    del /f /q "SDLGame_Web\game.js" "SDLGame_Web\game.wasm" >nul 2>&1
    :: -s LEGACY_GL_EMULATION=1
    em++ "src\main.cpp" "src\engine.cpp" -O3 -s -flto -ffunction-sections -fdata-sections -std=c++11 -pipe -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-write-strings -Wno-dollar-in-identifier-extension -DNDEBUG -s ASSERTIONS=1 -s EMULATE_FUNCTION_POINTER_CASTS=1 -s ALLOW_MEMORY_GROWTH=1 -s FULL_ES2=1 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS="['png']" -o "SDLGame_Web\game.js" %*
)
