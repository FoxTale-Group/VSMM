
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(APP_DISPLAY_NAME "Vintage Story Mod Manager")
configure_file(config.hpp.in config.hpp)

qt_add_executable(${PROJECT_NAME}
        main.cpp
        App.cpp
        ZipArchive.cpp
        ZipArchive.hpp
        ModEntry.cpp
        ModImageProvider.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE zip semver Qt6::Core Qt6::Quick Qt6::Qml)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../icons/*.svg") # Get all SVG icons
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../qml/*.qml") # Get all QML files

# Set alias for each QML file
foreach(qml_file ${QML_FILES})
    get_filename_component(file_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${file_name})
endforeach()

qt_add_qml_module(${PROJECT_NAME}
        URI vsmodchecker
        VERSION 1.0
        QML_FILES ${QML_FILES}
        RESOURCES ${ICON_FILES} ../icons/VSMM_Icon.png
        SOURCES
            ModListModel.cpp
            ModListModel.hpp
            ModManager.cpp
            ModManager.hpp
            ModSortFilterModel.cpp
            ModSortFilterModel.hpp
)