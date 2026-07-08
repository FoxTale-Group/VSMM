cmake_minimum_required(VERSION 4.3)
set(VSMM_RELEASE_VERSION "0.0.1" CACHE STRING "Project version")
project(VSMM
        DESCRIPTION "Mod Checker For Vintage Story"
        HOMEPAGE_URL "https://github.com/FoxTale-Group/VSMM"
        LANGUAGES CXX)
set(APP_DISPLAY_NAME "Vintage Story Mod Manager")
set(CMAKE_CXX_STANDARD 23)

if (MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif ()

find_package(libzip CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Quick Qml Network QuickControls2)

qt_standard_project_setup()

include(FetchContent)
FetchContent_Declare(
        cpp-semver
        GIT_REPOSITORY https://github.com/z4kn4fein/cpp-semver.git
        GIT_TAG v0.4.0)
FetchContent_MakeAvailable(cpp-semver)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib/vsmm")
add_subdirectory(src bin)
