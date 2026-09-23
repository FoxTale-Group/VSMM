project(GameMngrTests)

qt_add_executable(GameMngrUnitTest GameMngrUnitTest.cpp)
add_test(NAME GameMngrUnitTest COMMAND GameMngrUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/GameMngrUnitTest.junit.xml,junitxml)
add_dependencies(GameMngrUnitTest GameMngr TestMocks TestUtils)
target_link_libraries(GameMngrUnitTest PUBLIC Qt6::Test GameMngr TestMocks TestUtils)

qt_add_executable(GameMngrIntegrationTest GameMngrIntegrationTest.cpp)
add_test(NAME GameMngrIntegrationTest COMMAND GameMngrIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/GameMngrIntegrationTest.junit.xml,junitxml)
add_dependencies(GameMngrIntegrationTest GameMngr Config TestUtils)
target_link_libraries(GameMngrIntegrationTest PUBLIC Qt6::Test GameMngr Config TestUtils)

set_target_properties(GameMngrUnitTest GameMngrIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(GameMngrUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
    set_tests_properties(GameMngrIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()
