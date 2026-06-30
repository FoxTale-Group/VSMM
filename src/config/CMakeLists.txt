project(Config)

set(SOURCES Config.cpp Config.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core Qt6::Quick Qt6::Qml)
qt_add_qml_module(${PROJECT_NAME}
        URI Config
        VERSION 1.0
        SOURCES ${SOURCES})