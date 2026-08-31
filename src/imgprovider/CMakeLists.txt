project(ImgProvider)
include(GenerateExportHeader)

set(SOURCES ModImageProvider.cpp ModImageProvider.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_dependencies(${PROJECT_NAME} Interfaces)

target_include_directories(${PROJECT_NAME}
                            PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core Qt6::Quick Interfaces)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/vsmm)

generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME "${PROJECT_NAME}Export.hpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)