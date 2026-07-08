project(ZipArchive)
include(GenerateExportHeader)

set(SOURCES ZipArchive.cpp ZipArchive.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC libzip::zip Qt6::Core)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/vsmm)

generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME "${PROJECT_NAME}Export.hpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)