project(ModLoader VERSION 1.0)
include(GenerateExportHeader)

set(SOURCES ModLoader.cpp ModLoader.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_dependencies(${PROJECT_NAME} ImgProvider ModEntry Config ModStore ZipArchive)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core Qt6::Quick Qt6::Qml Qt6::Network ImgProvider ModEntry Config ModStore ZipArchive)
qt_add_qml_module(${PROJECT_NAME}
        URI ${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        SOURCES ${SOURCES})
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)

generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME "${PROJECT_NAME}Export.hpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)