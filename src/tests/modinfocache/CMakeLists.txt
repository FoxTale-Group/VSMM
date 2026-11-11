project(ModInfoCacheUnitTest)

set(SOURCES ModInfoCacheUnitTest.cpp)
qt_add_executable(${PROJECT_NAME} ${SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME}
        -o -,txt -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.junit.xml,junitxml)
add_dependencies(${PROJECT_NAME} ModInfoCache)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Test Qt6::Sql ModInfoCache TestUtils)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find the component DLLs at test run time.
if (WIN32)
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModInfoCache>;PATH=path_list_prepend:$<TARGET_FILE_DIR:Interfaces>;PATH=path_list_prepend:$<TARGET_FILE_DIR:ModEntry>")
else ()
# a zone with dst, so a date stored or read as local time would shift
set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT "TZ=Europe/Warsaw")
endif ()
