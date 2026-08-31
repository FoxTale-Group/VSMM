project(ModSortFilterModelUnitTest)

set(SOURCES ModSortFilterModelUnitTest.cpp ModSortFilterModelTestUtils.hpp)
qt_add_executable(${PROJECT_NAME} ${SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} ModSortFilterModel)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Test ModSortFilterModel)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

# Helps windows to find ModSortFilterModel.dll
if (WIN32)
set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ModSortFilterModel>")
endif ()
