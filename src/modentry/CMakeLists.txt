project(ModEntry)

set(SOURCES ModEntry.cpp ModEntry.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC semver Qt6::Core)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)