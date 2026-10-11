cmake_minimum_required(VERSION 3.24)
project(voltra_remote LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER MATCHES "zig\\.exe$")
    # Zig bundles the archive tools as subcommands, not separate llvm-ar binaries.
    set(CMAKE_AR "${CMAKE_CXX_COMPILER}")
    foreach(lang C CXX ASM)
        set(CMAKE_${lang}_ARCHIVE_CREATE "<CMAKE_AR> ar qc <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_${lang}_ARCHIVE_APPEND "<CMAKE_AR> ar q <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_${lang}_ARCHIVE_FINISH "<CMAKE_AR> ranlib <TARGET>")
    endforeach()
    set(CMAKE_ASM_COMPILER_ARG1 cc)
endif()

add_library(voltra_core
    src/protocol/FrameParser.cpp
    src/protocol/Parameters.cpp
    src/controller/VoltraController.cpp
    src/controller/TrainingApplication.cpp)
target_include_directories(voltra_core PUBLIC src)
# Every CMake artifact is offline. Embedded targets use separate PlatformIO sources.
target_compile_definitions(voltra_core PUBLIC VOLTRA_OFFLINE=1)
if(MSVC)
    target_compile_options(voltra_core PRIVATE /W4)
else()
    target_compile_options(voltra_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(packet_inspector tools/packet_inspector/main.cpp)
target_link_libraries(packet_inspector PRIVATE voltra_core)

option(VOLTRA_BUILD_SIMULATOR "Build offline Windows/macOS LVGL simulator" ON)
if(VOLTRA_BUILD_SIMULATOR)
    if(NOT WIN32 AND NOT APPLE)
        message(FATAL_ERROR "Desktop display adapters support Windows and macOS")
    endif()
    set(LVGL_DIR "${CMAKE_SOURCE_DIR}/.tools/lvgl-9.3.0" CACHE PATH "Pinned LVGL source directory")
    if(NOT EXISTS "${LVGL_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "LVGL 9.3.0 missing. See docs/DEVELOPMENT.md")
    endif()
    set(LV_BUILD_CONF_PATH "${CMAKE_SOURCE_DIR}/config/lv_conf.h" CACHE PATH "" FORCE)
    set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${LVGL_DIR}" "${CMAKE_BINARY_DIR}/lvgl")
    add_library(voltra_ui src/ui/UiController.cpp)
    target_link_libraries(voltra_ui PUBLIC voltra_core lvgl)
    if(APPLE)
        enable_language(OBJCXX)
        add_executable(voltra_simulator MACOSX_BUNDLE simulator/main_mac.mm)
        set_target_properties(voltra_simulator PROPERTIES
            OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES OBJCXX_EXTENSIONS NO
            MACOSX_BUNDLE_BUNDLE_NAME "VOLTRA Simulator"
            MACOSX_BUNDLE_GUI_IDENTIFIER "local.voltra.simulator"
            MACOSX_BUNDLE_BUNDLE_VERSION "1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0")
        target_compile_options(voltra_simulator PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
        target_link_libraries(voltra_simulator PRIVATE "-framework Cocoa" "-framework CoreGraphics")
    else()
        add_executable(voltra_simulator simulator/main.cpp)
        target_link_libraries(voltra_simulator PRIVATE gdi32 user32)
    endif()
    target_sources(voltra_simulator PRIVATE simulator/MockVoltra.cpp simulator/MockTransport.cpp)
    target_include_directories(voltra_simulator PRIVATE simulator)
    target_compile_definitions(voltra_simulator PRIVATE VOLTRA_SIMULATOR=1 NOMINMAX)
    target_link_libraries(voltra_simulator PRIVATE voltra_ui)
    # Explicit source lists: no board code, NimBLE or physical BLE library linked.
endif()
# New test code is deferred per PROJECT_REQUIREMENTS. No fake CTest targets.
