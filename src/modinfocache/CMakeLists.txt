project(ModInfoCache VERSION 1.0)
include(GenerateExportHeader)

set(SOURCES ModInfoCache.cpp ModInfoCache.hpp
        ModInfoCacheSql.hpp
        ModInfoCacheValues.cpp ModInfoCacheValues.hpp
        ModListWriter.cpp ModListWriter.hpp)
qt_add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_dependencies(${PROJECT_NAME} Interfaces)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core Qt6::Sql Interfaces)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/vsmm)

generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME "${PROJECT_NAME}Export.hpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)