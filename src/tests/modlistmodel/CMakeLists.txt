project(ModListModelUnitTest)

set(SOURCES ModListModelUnitTest.cpp)
qt_add_executable(${PROJECT_NAME} ${SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME}
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.junit.xml,junitxml)
add_dependencies(${PROJECT_NAME} ModListModel TestMocks TestUtils)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Test ModListModel TestMocks TestUtils)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModListModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()
