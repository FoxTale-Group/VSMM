project(GameMngrTests)

qt_add_executable(GameMngrUnitTest GameMngrUnitTest.cpp GameMngrTestUtils.hpp)
add_test(NAME GameMngrUnitTest COMMAND GameMngrUnitTest)
add_dependencies(GameMngrUnitTest GameMngr TestMocks)
target_link_libraries(GameMngrUnitTest PUBLIC Qt6::Test GameMngr TestMocks)

qt_add_executable(GameMngrIntegrationTest GameMngrIntegrationTest.cpp GameMngrTestUtils.hpp)
add_test(NAME GameMngrIntegrationTest COMMAND GameMngrIntegrationTest)
add_dependencies(GameMngrIntegrationTest GameMngr Config)
target_link_libraries(GameMngrIntegrationTest PUBLIC Qt6::Test GameMngr Config)

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
