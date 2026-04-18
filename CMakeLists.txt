cmake_minimum_required(VERSION 3.23.0)
project(Cadmium VERSION 1.0.0 LANGUAGES CXX)

message("[Cadmium] - Configuring...")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
