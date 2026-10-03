project(ModLoaderTests)

qt_add_executable(ModLoaderUnitTest ModLoaderUnitTest.cpp)
add_test(NAME ModLoaderUnitTest COMMAND ModLoaderUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModLoaderUnitTest.junit.xml,junitxml)
add_dependencies(ModLoaderUnitTest ModLoader TestMocks TestUtils)
# libzip comes in through ZipArchive, the test builds its own fixtures with it
target_link_libraries(ModLoaderUnitTest PUBLIC Qt6::Test ModLoader TestMocks TestUtils)

qt_add_executable(ModLoaderIntegrationTest ModLoaderIntegrationTest.cpp)
add_test(NAME ModLoaderIntegrationTest COMMAND ModLoaderIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModLoaderIntegrationTest.junit.xml,junitxml)
add_dependencies(ModLoaderIntegrationTest ModLoader HttpClient ModStore Config GameMngr TestUtils)
target_link_libraries(ModLoaderIntegrationTest PUBLIC
        Qt6::Test Qt6::Network ModLoader HttpClient ModStore Config GameMngr TestUtils)

set_target_properties(ModLoaderUnitTest ModLoaderIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# a request that never settles would otherwise hang the run
set_tests_properties(ModLoaderIntegrationTest PROPERTIES
        ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=60000"
        TIMEOUT 120)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(ModLoaderUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModLoader>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ZipArchive>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
    set_tests_properties(ModLoaderIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModLoader>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ZipArchive>;PATH=path_list_prepend:$<TARGET_FILE_DIR:HttpClient>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()

# Response is filled with designated initializers that leave the rest at their defaults
if (NOT MSVC)
    target_compile_options(ModLoaderIntegrationTest PRIVATE -Wno-missing-field-initializers)
endif ()
