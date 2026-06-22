cmake_minimum_required(VERSION 4.3)
project(VSModChecker)
set(CMAKE_CXX_STANDARD 26)

find_package(libzip REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpr REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Quick Qml)
qt_standard_project_setup()

include(FetchContent)
FetchContent_Declare(
        cpp-semver
        GIT_REPOSITORY https://github.com/z4kn4fein/cpp-semver.git
        GIT_TAG v0.4.0)
FetchContent_MakeAvailable(cpp-semver)

add_subdirectory(src)
