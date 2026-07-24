cmake_minimum_required(VERSION 3.23.0)
project(Cadmium VERSION 1.0.0 LANGUAGES C CXX)

message("[Cadmium] - Configuring...")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
message("[Cadmium] - Emscripten build: relaxing HarfBuzz warning->error flags")
# Prevent HarfBuzz warnings from being promoted to errors by em++/clang
# Suppressed: unused-template, unused-function, nontrivial-memcall, array-bounds
add_compile_options(
  -Wno-error=unused-template -Wno-unused-template
  -Wno-error=unused-function -Wno-unused-function
  -Wno-error=nontrivial-memcall -Wno-nontrivial-memcall
  -Wno-error=array-bounds -Wno-array-bounds
)
endif()

option(CADMIUM_IMGUI "Enable ImGui support" ON)

include(cmake/helpers.cmake)
message("[Cadmium] - Fetching Dependencies")
include(cmake/dependencies.cmake)
message("[Cadmium] - Fetched!")


add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(sandbox)

if(NOT EMSCRIPTEN)
enable_testing()
add_subdirectory(tests)
endif()
