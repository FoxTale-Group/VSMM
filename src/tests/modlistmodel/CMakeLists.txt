project(ModListModelTests)

qt_add_executable(ModListModelUnitTest ModListModelUnitTest.cpp)
add_test(NAME ModListModelUnitTest COMMAND ModListModelUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModListModelUnitTest.junit.xml,junitxml)
add_dependencies(ModListModelUnitTest ModListModel TestMocks TestUtils)
target_link_libraries(ModListModelUnitTest PUBLIC Qt6::Test ModListModel TestMocks TestUtils)

qt_add_executable(ModListModelIntegrationTest ModListModelIntegrationTest.cpp)
add_test(NAME ModListModelIntegrationTest COMMAND ModListModelIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModListModelIntegrationTest.junit.xml,junitxml)
add_dependencies(ModListModelIntegrationTest ModListModel ModStore Config GameMngr TestUtils)
target_link_libraries(ModListModelIntegrationTest PUBLIC Qt6::Test ModListModel ModStore Config GameMngr TestUtils)

set_target_properties(ModListModelUnitTest ModListModelIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(ModListModelUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModListModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
    set_tests_properties(ModListModelIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModListModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()
