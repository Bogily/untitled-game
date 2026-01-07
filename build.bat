@echo off
echo ========================================
echo Building ZeldaLikeGame
echo ========================================

:: Check if mingw32-make is in PATH
where mingw32-make >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: mingw32-make not found in PATH!
    echo Please add MinGW bin directory to your PATH
    echo Example: C:\MinGW\bin or C:\msys64\mingw64\bin
    pause
    exit /b 1
)

:: Check if g++ is in PATH
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: g++ not found in PATH!
    echo Please add MinGW bin directory to your PATH
    pause
    exit /b 1
)

:: Create build directory if it doesn't exist
if not exist "build" mkdir build

:: Navigate to build directory
cd build

:: Clean CMake cache to avoid generator conflicts
if exist "CMakeCache.txt" (
    echo Cleaning CMake cache...
    del /F /Q CMakeCache.txt >nul 2>&1
)
if exist "CMakeFiles" (
    rmdir /S /Q CMakeFiles >nul 2>&1
)

:: Run CMake to generate MinGW Makefiles
echo Running CMake...
cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

:: Build the project
echo Building project...
cmake --build .
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

:: Attempt to locate and copy raylib DLLs into this build directory
echo Looking for raylib DLLs to copy into the build folder...
set "COPIED=0"
for /r "%CD%" %%F in (raylib*.dll libraylib*.dll) do if exist "%%F" (
    echo Copying %%~nxF to %CD%\
    copy /Y "%%F" "%CD%\" >nul
    set COPIED=1
    goto DLL_COPIED
)
:DLL_COPIED
if "%COPIED%"=="0" (
    echo Warning: raylib DLL not found automatically.
    echo You may need to copy the produced raylib DLL into this folder or add it to PATH.
) else (
    echo Copied raylib DLL into build folder.
)

:: Go back to root directory
cd ..

echo ========================================
echo Build successful!
echo Executable is in: build\ZeldaLikeGame.exe
echo ========================================
echo.
echo To run the game, execute: build\ZeldaLikeGame.exe
pause
