@echo off
echo ========================================
echo Building ZeldaLikeGame
echo ========================================

:: Create build directory if it doesn't exist
if not exist "build" mkdir build

:: Configure with CMake
echo Configuring with CMake...
cmake -B build
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

:: Build the project
echo Building project...
cmake --build build --config RelWithDebInfo --parallel
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo ========================================
echo Build successful!
if exist "build\RelWithDebInfo\ZeldaLikeGame.exe" (
    echo Executable is in: build\RelWithDebInfo\ZeldaLikeGame.exe
) else (
    echo Executable is in: build\ZeldaLikeGame.exe
)
echo ========================================
echo.
pause
