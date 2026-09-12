@echo off
REM ============================================================
REM  build_web.bat - Build Quantum Garden web version (WASM)
REM
REM  Requirements:
REM    1. Emscripten SDK at D:\emsdk (uses portable Python 3.12 at
REM       D:\emsdk_dl\py312 because emsdk needs Python >= 3.10)
REM    2. Web raylib static lib already built:
REM       D:\emsdk_dl\raylib_x\raylib-5.5\src\libraylib.a
REM  Output: web\game_v12.js + game_v12.wasm + game_v12.data
REM  Update web\site.html when bumping the version, then copy it to index.html.
REM  Run after build: run_web.bat
REM ============================================================
setlocal
set EMSDK=D:\emsdk
set PY=D:\emsdk_dl\py312\python.exe
set EMXX=%EMSDK%\upstream\emscripten\em++.py
set RAYLIB_DIR=D:\emsdk_dl\raylib_x\raylib-5.5\src

set EMSDK_CONFIG=%EMSDK%\.emscripten
set EM_CACHE=%EMSDK%\upstream\emscripten\cache
REM emcc spawns native launcher file_packager.exe at link stage; it resolves
REM "python" via PATH. If PATH holds system Python 3.9 it fails with
REM "emscripten requires python 3.10 or above". Put portable Python 3.12
REM first on PATH and declare EMSDK_PYTHON explicitly.
set "EMSDK_PYTHON=%PY%"
set "PATH=D:\emsdk_dl\py312;%PATH%"

cd /d %~dp0

"%PY%" "%EMXX%" -std=c++17 -O2 ^
  src\main.cpp src\data.cpp src\state.cpp src\audio.cpp src\util.cpp ^
  src\save.cpp src\achievements.cpp src\render.cpp src\game.cpp src\scenes.cpp ^
  -I%RAYLIB_DIR% -L%RAYLIB_DIR% -lraylib ^
  -o web\game_v12.js ^
  --preload-file web\fonts\font.ttf@/fonts/font.ttf ^
  --preload-file web\fonts\font_title.ttf@/fonts/font_title.ttf ^
  --preload-file web\text.bin@/data/text.bin ^
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728 ^
  -sASYNCIFY -sFORCE_FILESYSTEM=1 -lidbfs.js ^
  -sEXPORTED_RUNTIME_METHODS=ccall,HEAPF32,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF64 ^
  -sENVIRONMENT=web -sUSE_GLFW=3 ^
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
echo BUILD_RESULT=%ERRORLEVEL%
endlocal
