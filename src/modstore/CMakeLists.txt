project(ModStore VERSION 1.0)

set(SOURCES ModStore.cpp ModStore.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_dependencies(ModEntry)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core Qt6::Quick Qt6::Qml ModEntry)
qt_add_qml_module(${PROJECT_NAME}
        URI ${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        SOURCES ${SOURCES})
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)