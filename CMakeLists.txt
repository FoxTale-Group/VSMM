cmake_minimum_required(VERSION 4.2)
set(VSMM_RELEASE_VERSION "0.2.0" CACHE STRING "Project version" FORCE)
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

# Emit .qmlls.ini next to the QML sources, pointing the QML Language Server at this build
# directory. Without it qmlls cannot find the generated qmldir/.qmltypes and silently
# returns ZERO completions -- no autocompletion for Theme.colors.*, VSMMStyle types, or any
# of the C++-backed singletons. The generated files hold absolute paths, so they are
# gitignored.
set(QT_QML_GENERATE_QMLLS_INI ON)

qt_standard_project_setup()

include(FetchContent)
FetchContent_Declare(
        semver
        GIT_REPOSITORY https://github.com/Neargye/semver.git
        GIT_TAG v1.0.0)
FetchContent_MakeAvailable(semver)

if (MSVC)
    add_compile_options(/W4 /WX)
else ()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-Wno-unused-parameter)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    add_compile_definitions(QT_NO_DEBUG_OUTPUT)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPOResult OUTPUT IPOOutput)
    if (IPOResult)
        message(STATUS "IPO is supported")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(STATUS "IPO is not supported: ${IPOOutput}")
    endif ()

    if (NOT MSVC)
        add_link_options(-Wl,--strip-all)
    endif ()
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    add_compile_definitions(DEBUG)

    include(CTest)
    set(CMAKE_CTEST_ARGUMENTS -V)
endif()

set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib/vsmm")
add_subdirectory(src bin)