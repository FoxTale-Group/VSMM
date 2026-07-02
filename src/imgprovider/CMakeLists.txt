project(ImgProvider)

set(SOURCES ModImageProvider.cpp ModImageProvider.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core Qt6::Quick)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)