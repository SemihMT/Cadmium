# cmake/lua.cmake
# Produces the lua::lua imported target.

include(FetchContent)

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.7
)

# FetchContent_MakeAvailable populates the content (downloads it)
FetchContent_MakeAvailable(lua)

# At this point the source directory is available as ${lua_SOURCE_DIR}
file(GLOB LUA_ALL_SOURCES CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/*.c")

# Exclude the standalone interpreters and the unity build file
list(FILTER LUA_ALL_SOURCES EXCLUDE REGEX
    ".*(lua|luac|onelua)\\.c$")

add_library(lua_static STATIC ${LUA_ALL_SOURCES})

target_include_directories(lua_static
    PUBLIC "${lua_SOURCE_DIR}"
)

# Platform-specific link dependencies
if(UNIX AND NOT APPLE)
    target_link_libraries(lua_static PUBLIC m dl)
    target_compile_definitions(lua_static PRIVATE LUA_USE_LINUX)
elseif(APPLE)
    target_compile_definitions(lua_static PRIVATE LUA_USE_MACOSX)
endif()

# Silence warnings from third-party code
if(MSVC)
    target_compile_options(lua_static PRIVATE /W0)
else()
    target_compile_options(lua_static PRIVATE -w)
endif()

# Emscripten: disable POSIX features that aren't available
if(EMSCRIPTEN)
    target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX=0)
endif()

# Position independent code is good practice
set_target_properties(lua_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Alias to the conventional namespaced target name
add_library(lua::lua ALIAS lua_static)
