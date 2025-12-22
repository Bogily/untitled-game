cmake_minimum_required(VERSION 3.11)

project(ZeldaLikeGame VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build raylib as a shared library so GLAD symbols are exported on Windows
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

# --- Fetch Raylib ---
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/master.tar.gz
)
FetchContent_MakeAvailable(raylib)

# --- Source Files ---
# Recursively find sources (for this simple example structure)
# In a real production environment, listing files manually is often preferred to avoid globbing issues.
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- Include Directories ---
target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# --- Assets Copying (Optional but helpful) ---
# Copies the assets folder to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
