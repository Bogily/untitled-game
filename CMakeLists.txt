cmake_minimum_required(VERSION 4.0)

project(ZeldaLikeGame VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
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
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
set(CMAKE_POLICY_VERSION_MINIMUM 4.0 CACHE INTERNAL "")
FetchContent_MakeAvailable(glad)

# Lua (scripting for scene loading and game logic)
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(lua)

# Freetype (required by RmlUi)
FetchContent_Declare(
    Freetype
    URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(FT_ENABLE_ERROR_STRINGS ON CACHE BOOL "" FORCE)
set(FT_WITH_ZLIB ON CACHE BOOL "" FORCE)
set(FT_WITH_PNG ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Freetype)

# Create an alias for RmlUi to find
if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

# RmlUi (HTML/CSS based UI)
set(RMLUI_ENABLE_LUA OFF CACHE BOOL "Disable RmlUi Lua bindings" FORCE)
set(RMLUI_ENABLE_SAMPLES OFF CACHE BOOL "Disable RmlUi samples" FORCE)
set(RMLUI_ENABLE_TESTS OFF CACHE BOOL "Disable RmlUi tests" FORCE)
set(RMLUI_ENABLE_PROFILER OFF CACHE BOOL "Disable RmlUi profiler" FORCE)
FetchContent_Declare(
    RmlUi
    GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
    GIT_TAG 6.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(RmlUi)

# Patch RmlUi 6.0: ensure robin_hood.h includes <cstdint> (toolchain-specific missing include)
set(RMLUI_ROBIN_HOOD "${RmlUi_SOURCE_DIR}/Include/RmlUi/Core/Containers/robin_hood.h")
if(EXISTS "${RMLUI_ROBIN_HOOD}")
    file(READ "${RMLUI_ROBIN_HOOD}" RMLUI_ROBIN_HOOD_CONTENT)
    if(NOT RMLUI_ROBIN_HOOD_CONTENT MATCHES "#include <cstdint>")
        string(REPLACE "#include <limits>\n" "#include <limits>\n#include <cstdint>\n" RMLUI_ROBIN_HOOD_CONTENT "${RMLUI_ROBIN_HOOD_CONTENT}")
        file(WRITE "${RMLUI_ROBIN_HOOD}" "${RMLUI_ROBIN_HOOD_CONTENT}")
    endif()
endif()

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

add_library(raylib_rmlui STATIC
    ${CMAKE_SOURCE_DIR}/src/ui/rmlui/raylibFileInterface.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/rmlui/raylibRenderInterface.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/rmlui/raylibSystemInterface.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/rmlui/raylibRmlUi.cpp
)

target_include_directories(raylib_rmlui PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${RmlUi_SOURCE_DIR}/Include
)
target_link_libraries(raylib_rmlui PUBLIC raylib rmlui_core rmlui_debugger)

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- Include Directories ---
target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- Link Libraries ---
# raylib provides most functionality, but we also need glad for compute/SSBO OpenGL calls
target_link_libraries(${PROJECT_NAME} PRIVATE raylib glad lua_static raylib_rmlui)
# Make sure the glad include dir is available to the project (FetchContent provides glad source)
target_include_directories(${PROJECT_NAME} PRIVATE ${glad_SOURCE_DIR}/include)
# Make sure Lua headers are available
target_include_directories(${PROJECT_NAME} PRIVATE ${lua_SOURCE_DIR}/src)

# --- Copy DLLs to build directory for Windows ---
if(WIN32)
    # Copy raylib DLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:raylib>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
    )

    # Copy glad DLL
    if(TARGET glad)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:glad>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        )
    endif()

    # Copy freetype DLL
    if(TARGET freetype)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:freetype>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        )
    endif()

    # Copy rmlui DLLs
    if(TARGET rmlui_core)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:rmlui_core>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        )
    endif()

    if(TARGET rmlui_debugger)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:rmlui_debugger>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        )
    endif()
endif()

# --- Assets Copying (Optional but helpful) ---
# Copies the assets folder to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
