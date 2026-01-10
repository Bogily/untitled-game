cmake_minimum_required(VERSION 3.11)

project(ZeldaLikeGame VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build raylib as a shared library so GLAD symbols are exported on Windows
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

# Set OpenGL version to 4.3 for SSBO support
set(GRAPHICS "GRAPHICS_API_OPENGL_43" CACHE STRING "OpenGL version for raylib" FORCE)

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

# Lua (scripting for scene loading and game logic)
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
)
FetchContent_MakeAvailable(lua)

# Build Lua as a library
if(NOT TARGET lua_static)
    add_library(lua_static STATIC
        ${lua_SOURCE_DIR}/src/lapi.c
        ${lua_SOURCE_DIR}/src/lauxlib.c
        ${lua_SOURCE_DIR}/src/lbaselib.c
        ${lua_SOURCE_DIR}/src/lcode.c
        ${lua_SOURCE_DIR}/src/lcorolib.c
        ${lua_SOURCE_DIR}/src/lctype.c
        ${lua_SOURCE_DIR}/src/ldblib.c
        ${lua_SOURCE_DIR}/src/ldebug.c
        ${lua_SOURCE_DIR}/src/ldo.c
        ${lua_SOURCE_DIR}/src/ldump.c
        ${lua_SOURCE_DIR}/src/lfunc.c
        ${lua_SOURCE_DIR}/src/lgc.c
        ${lua_SOURCE_DIR}/src/linit.c
        ${lua_SOURCE_DIR}/src/liolib.c
        ${lua_SOURCE_DIR}/src/llex.c
        ${lua_SOURCE_DIR}/src/lmathlib.c
        ${lua_SOURCE_DIR}/src/lmem.c
        ${lua_SOURCE_DIR}/src/loadlib.c
        ${lua_SOURCE_DIR}/src/lobject.c
        ${lua_SOURCE_DIR}/src/lopcodes.c
        ${lua_SOURCE_DIR}/src/loslib.c
        ${lua_SOURCE_DIR}/src/lparser.c
        ${lua_SOURCE_DIR}/src/lstate.c
        ${lua_SOURCE_DIR}/src/lstring.c
        ${lua_SOURCE_DIR}/src/lstrlib.c
        ${lua_SOURCE_DIR}/src/ltable.c
        ${lua_SOURCE_DIR}/src/ltablib.c
        ${lua_SOURCE_DIR}/src/ltm.c
        ${lua_SOURCE_DIR}/src/lundump.c
        ${lua_SOURCE_DIR}/src/lutf8lib.c
        ${lua_SOURCE_DIR}/src/lvm.c
        ${lua_SOURCE_DIR}/src/lzio.c
    )
    target_include_directories(lua_static PUBLIC ${lua_SOURCE_DIR}/src)
    # Disable lua.c and luac.c (standalone executables)
    set_source_files_properties(${lua_SOURCE_DIR}/src/lua.c ${lua_SOURCE_DIR}/src/luac.c PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

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
target_link_libraries(${PROJECT_NAME} PRIVATE raylib glad lua_static)
# Make sure the glad include dir is available to the project (FetchContent provides glad source)
target_include_directories(${PROJECT_NAME} PRIVATE ${glad_SOURCE_DIR}/include)
# Make sure Lua headers are available
target_include_directories(${PROJECT_NAME} PRIVATE ${lua_SOURCE_DIR}/src)

# --- Assets Copying (Optional but helpful) ---
# Copies the assets folder to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
