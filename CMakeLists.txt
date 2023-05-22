cmake_minimum_required(VERSION 3.10)

project(VisualDebugger
    VERSION 0.1.0
    DESCRIPTION "visual debugger for PM"
    LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(3rdlibs)

aux_source_directory(src/lib SRC)
add_library(dbglib STATIC ${SRC})
target_link_libraries(dbglib PUBLIC glm glfw glad)
target_include_directories(dbglib PUBLIC include)
target_compile_features(dbglib PUBLIC cxx_std_17)

aux_source_directory(src/debugger DBG_SRC)
add_executable(debugger ${DBG_SRC})
target_link_libraries(debugger PUBLIC dbglib)

# unittest
enable_testing()
add_subdirectory(test)
