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

# Formatting target
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
  file(
    GLOB_RECURSE
    ALL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  )

  add_custom_target(
    format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file ${ALL_SOURCE_FILES}
    COMMENT "Running clang-format on source files"
    VERBATIM
  )

  message(STATUS "clang-format found: ${CLANG_FORMAT_EXECUTABLE}")
  message(STATUS "  Run 'cmake --build . --target format' to format code")
else()
  message(STATUS "clang-format not found - format target not available")
endif()

# Linting target
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
  file(
    GLOB_RECURSE
    ALL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  )

  add_custom_target(
    lint
    COMMAND
    ${CLANG_TIDY_EXECUTABLE}
    -p
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ALL_SOURCE_FILES}
    --
    -std=c++23
    -I${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Running clang-tidy on source files"
    VERBATIM
  )

  add_custom_target(
    lint-fix
    COMMAND
    ${CLANG_TIDY_EXECUTABLE}
    -p
    ${CMAKE_CURRENT_BINARY_DIR}
    -fix
    ${ALL_SOURCE_FILES}
    --
    -std=c++23
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Running clang-tidy with auto-fix on source files"
    VERBATIM
  )

  message(STATUS "clang-tidy found: ${CLANG_TIDY_EXECUTABLE}")
  message(STATUS "  Run 'cmake --build . --target lint' to lint code")
  message(
    STATUS "  Run 'cmake --build . --target lint-fix' to lint and auto-fix code"
  )
else()
  message(STATUS "clang-tidy not found - lint target not available")
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui Quick Svg Network WebSockets DBus Test Multimedia)
qt_standard_project_setup()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
  gstreamer-1.0
  gstreamer-webrtc-1.0
  gstreamer-sdp-1.0
  gstreamer-video-1.0
  gstreamer-app-1.0
)
pkg_check_modules(PW REQUIRED
  libpipewire-0.3
)

option(BUILD_TESTING "Build test executables" ON)
include(CTest)

add_subdirectory(src)
