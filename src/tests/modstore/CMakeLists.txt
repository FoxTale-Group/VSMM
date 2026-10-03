project(ModStoreTests)

qt_add_executable(ModStoreUnitTest ModStoreUnitTest.cpp ModStoreTestUtils.hpp)
add_test(NAME ModStoreUnitTest COMMAND ModStoreUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModStoreUnitTest.junit.xml,junitxml)
add_dependencies(ModStoreUnitTest ModStore TestMocks TestUtils)
target_link_libraries(ModStoreUnitTest PUBLIC Qt6::Test ModStore TestMocks TestUtils)

qt_add_executable(ModStoreIntegrationTest ModStoreIntegrationTest.cpp ModStoreTestUtils.hpp)
add_test(NAME ModStoreIntegrationTest COMMAND ModStoreIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModStoreIntegrationTest.junit.xml,junitxml)
add_dependencies(ModStoreIntegrationTest ModStore Config GameMngr TestUtils)
target_link_libraries(ModStoreIntegrationTest PUBLIC Qt6::Test ModStore Config GameMngr TestUtils)

set_target_properties(ModStoreUnitTest ModStoreIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(ModStoreUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
    set_tests_properties(ModStoreIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()
