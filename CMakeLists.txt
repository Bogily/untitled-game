cmake_minimum_required(VERSION 3.11)

project(ZeldaLikeGame VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build raylib as a shared library so GLAD symbols are exported on Windows
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

# --- Fetch dependencies ---
include(FetchContent)

# Raylib (windowing, input, base rendering)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/master.tar.gz
)
FetchContent_MakeAvailable(raylib)

# GLAD (OpenGL loader used for compute shaders/SSBOs)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
# Some older third-party projects require compatibility with older CMake minimal versions.
# Set this to allow those projects to configure under newer CMake without failing.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE INTERNAL "")
FetchContent_MakeAvailable(glad)

# --- Source Files ---
# Recursively find sources (for this simple example structure)
# In a real production environment, listing files manually is often preferred to avoid globbing issues.
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- Include Directories ---
target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- Link Libraries ---
# raylib provides most functionality, but we also need glad for compute/SSBO OpenGL calls
target_link_libraries(${PROJECT_NAME} PRIVATE raylib glad)
# Make sure the glad include dir is available to the project (FetchContent provides glad source)
target_include_directories(${PROJECT_NAME} PRIVATE ${glad_SOURCE_DIR}/include)

# --- Assets Copying (Optional but helpful) ---
# Copies the assets folder to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
