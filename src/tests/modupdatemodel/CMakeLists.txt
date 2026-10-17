project(ModUpdateModelTests)

qt_add_executable(ModUpdateModelUnitTest ModUpdateModelUnitTest.cpp)
add_test(NAME ModUpdateModelUnitTest COMMAND ModUpdateModelUnitTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModUpdateModelUnitTest.junit.xml,junitxml)
add_dependencies(ModUpdateModelUnitTest ModUpdateModel TestMocks TestUtils)
target_link_libraries(ModUpdateModelUnitTest PUBLIC Qt6::Test ModUpdateModel TestMocks TestUtils)

qt_add_executable(ModUpdateModelIntegrationTest ModUpdateModelIntegrationTest.cpp)
add_test(NAME ModUpdateModelIntegrationTest COMMAND ModUpdateModelIntegrationTest
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/ModUpdateModelIntegrationTest.junit.xml,junitxml)
add_dependencies(ModUpdateModelIntegrationTest ModUpdateModel ModLoader HttpClient ModStore Config GameMngr TestUtils)
target_link_libraries(ModUpdateModelIntegrationTest PUBLIC
        Qt6::Test Qt6::Network ModUpdateModel ModLoader HttpClient ModStore Config GameMngr TestUtils)

set_target_properties(ModUpdateModelUnitTest ModUpdateModelIntegrationTest PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# a request that never finishes would otherwise hang the run
set_tests_properties(ModUpdateModelIntegrationTest PROPERTIES
        ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=60000"
        TIMEOUT 120)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
    set_tests_properties(ModUpdateModelUnitTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModUpdateModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
    set_tests_properties(ModUpdateModelIntegrationTest PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModUpdateModel>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModLoader>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ZipArchive>;PATH=path_list_prepend:$<TARGET_FILE_DIR:HttpClient>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModStore>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Config>;PATH=path_list_prepend:$<TARGET_FILE_DIR:GameMngr>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()

# Response is filled with designated initializers that leave the rest at their defaults
if (NOT MSVC)
    target_compile_options(ModUpdateModelIntegrationTest PRIVATE -Wno-missing-field-initializers)
endif ()
