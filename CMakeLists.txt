cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(application LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# need for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# setup ccache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
	set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
	set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui Quick QuickControls2)
qt_standard_project_setup()

add_subdirectory(src)
