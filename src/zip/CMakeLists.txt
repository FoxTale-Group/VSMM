project(ZipArchive)

set(SOURCES ZipArchive.cpp ZipArchive.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC zip Qt6::Core)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)