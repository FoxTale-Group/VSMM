project(GameMngrTest)

find_package(Qt6 REQUIRED COMPONENTS Test)

set(SOURCES GameMngrTest.cpp)
qt_add_executable(${PROJECT_NAME} ${SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} GameMngr)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Test GameMngr)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find GameMngr.dll and Config.dll
if (WIN32)
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>")
endif ()
