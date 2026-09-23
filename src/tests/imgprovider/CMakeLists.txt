project(ModImageProviderUnitTest)

set(SOURCES ModImageProviderUnitTest.cpp)
qt_add_executable(${PROJECT_NAME} ${SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME}
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.junit.xml,junitxml)
add_dependencies(${PROJECT_NAME} ImgProvider TestMocks)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Test ImgProvider TestMocks)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# aReceiverMayCallBackIntoTheProviderWhileHandlingImageDownloaded deadlocks rather than fails if the
# provider ever emits under its mutex, so bound it. QtTest's own watchdog reports which function
# blocked and dumps a stack trace, the ctest cap is only the backstop for a hang outside one.
# Everything here is mocked, the whole suite runs in about two seconds.
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=60000"
        TIMEOUT 120)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ImgProvider>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>")
endif ()
