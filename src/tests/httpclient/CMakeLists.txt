project(HttpClientUnitTest)

set(SOURCES HttpClientUnitTest.cpp HttpClientTestUtils.hpp)
qt_add_executable(${PROJECT_NAME} ${SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} HttpClient)

# Qt6::Network for the loopback server standing in for the mod api
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Test Qt6::Network HttpClient)

# APP_NAME and APP_VERSION for the user agent assertion, configure_file puts constants.hpp in src's binary dir
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../..)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:HttpClient>")
endif ()
