cmake_minimum_required(VERSION 4.3)
project(VSModChecker
        VERSION 0.0.1
        DESCRIPTION "Mod Checker For Vintage Story"
        HOMEPAGE_URL "https://github.com/Amaroq7/VSModChecker"
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

find_package(libzip REQUIRED)
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

add_subdirectory(src bin)
