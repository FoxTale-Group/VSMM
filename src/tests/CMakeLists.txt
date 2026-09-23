project(Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)

add_library(TestMocks INTERFACE)
target_include_directories(TestMocks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
target_link_libraries(TestMocks INTERFACE Interfaces ModEntry)

add_library(TestUtils INTERFACE)
target_include_directories(TestUtils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/utils)
target_link_libraries(TestUtils INTERFACE Qt6::Test ModEntry)

add_subdirectory(config)
add_subdirectory(modentry)
add_subdirectory(modstore)
add_subdirectory(gamemngr)
add_subdirectory(httpclient)
add_subdirectory(imgprovider)
add_subdirectory(zip)
add_subdirectory(modsortfiltermodel)
add_subdirectory(modlistmodel)
