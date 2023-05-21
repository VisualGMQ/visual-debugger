cmake_minimum_required(VERSION 3.10)

project(VisualDebugger
    VERSION 0.1.0
    DESCRIPTION "visual debugger for PM"
    LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(3rdlibs)

aux_source_directory(src SRC)
add_executable(debugger ${SRC})
target_link_libraries(debugger PRIVATE glm glfw glad)
target_include_directories(debugger PRIVATE include)
target_compile_features(debugger PRIVATE cxx_std_17)