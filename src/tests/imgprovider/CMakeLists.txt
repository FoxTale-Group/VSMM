project(ModImageProviderTests)

qt_add_executable(ModImageProviderUnitTest ModImageProviderUnitTest.cpp)
add_test(NAME ModImageProviderUnitTest COMMAND ModImageProviderUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModImageProviderUnitTest.junit.xml,junitxml)
add_dependencies(ModImageProviderUnitTest ImgProvider TestMocks TestUtils)
target_link_libraries(ModImageProviderUnitTest PUBLIC Qt6::Test ImgProvider TestMocks TestUtils)

# Qt6::Network for the loopback server standing in for the mod api
qt_add_executable(ModImageProviderIntegrationTest ModImageProviderIntegrationTest.cpp)
add_test(NAME ModImageProviderIntegrationTest COMMAND ModImageProviderIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModImageProviderIntegrationTest.junit.xml,junitxml)
add_dependencies(ModImageProviderIntegrationTest ImgProvider HttpClient ModStore Config GameMngr TestUtils)
target_link_libraries(ModImageProviderIntegrationTest PUBLIC
        Qt6::Test Qt6::Network ImgProvider HttpClient ModStore Config GameMngr TestUtils)

set_target_properties(ModImageProviderUnitTest ModImageProviderIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# aReceiverMayCallBackIntoTheProviderWhileHandlingImageDownloaded deadlocks rather than fails if the
# provider ever emits under its mutex, so bound it. QtTest's own watchdog reports which function
# blocked and dumps a stack trace, the ctest cap is only the backstop for a hang outside one.
# Everything here is mocked, the whole suite runs in about two seconds.
set_tests_properties(ModImageProviderUnitTest PROPERTIES
        ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=60000"
        TIMEOUT 120)
# same backstop, a download that never settles would otherwise hang the run
set_tests_properties(ModImageProviderIntegrationTest PROPERTIES
        ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=60000"
        TIMEOUT 120)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(ModImageProviderUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ImgProvider>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
    set_tests_properties(ModImageProviderIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ImgProvider>;PATH=path_list_prepend:$<TARGET_FILE_DIR:HttpClient>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()

# Response is filled with designated initializers that leave the rest at their defaults
if (NOT MSVC)
    target_compile_options(ModImageProviderIntegrationTest PRIVATE -Wno-missing-field-initializers)
endif ()
