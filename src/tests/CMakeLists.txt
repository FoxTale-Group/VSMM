project(Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)

add_library(TestMocks INTERFACE)
target_include_directories(TestMocks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
target_link_libraries(TestMocks INTERFACE Interfaces)

add_subdirectory(config)
add_subdirectory(modentry)
add_subdirectory(gamemngr)
add_subdirectory(httpclient)
add_subdirectory(zip)
