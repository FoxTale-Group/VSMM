cmake_minimum_required(VERSION 4.2)
set(VSMM_RELEASE_VERSION "0.2.0-alpha.2" CACHE STRING "Project version" FORCE)
project(VSMM
        DESCRIPTION "Mod Manager For Vintage Story"
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
        semver
        GIT_REPOSITORY https://github.com/Neargye/semver.git
        GIT_TAG v1.0.0-rc)
FetchContent_MakeAvailable(semver)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

include(CTest)
set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib/vsmm")
add_subdirectory(src bin)