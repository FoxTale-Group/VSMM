project(ModSortFilterModelTests)

qt_add_executable(ModSortFilterModelUnitTest ModSortFilterModelUnitTest.cpp ModSortFilterModelTestUtils.hpp)
add_test(NAME ModSortFilterModelUnitTest COMMAND ModSortFilterModelUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModSortFilterModelUnitTest.junit.xml,junitxml)
add_dependencies(ModSortFilterModelUnitTest ModSortFilterModel)
target_link_libraries(ModSortFilterModelUnitTest PUBLIC Qt6::Test ModSortFilterModel)

qt_add_executable(ModSortFilterModelIntegrationTest ModSortFilterModelIntegrationTest.cpp)
add_test(NAME ModSortFilterModelIntegrationTest COMMAND ModSortFilterModelIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModSortFilterModelIntegrationTest.junit.xml,junitxml)
add_dependencies(ModSortFilterModelIntegrationTest ModSortFilterModel ModListModel ModStore Config GameMngr TestUtils)
target_link_libraries(ModSortFilterModelIntegrationTest PUBLIC
        Qt6::Test ModSortFilterModel ModListModel ModStore Config GameMngr TestUtils)

set_target_properties(ModSortFilterModelUnitTest ModSortFilterModelIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(ModSortFilterModelUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModSortFilterModel>")
    set_tests_properties(ModSortFilterModelIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModSortFilterModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModListModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()
