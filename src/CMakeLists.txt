qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

configure_file(config.hpp.in config.hpp)

set(EXECUTABLES
        main.cpp
        App.cpp
        ZipArchive.cpp
        ModEntry.cpp
        ModImageProvider.cpp
        ModListModel.cpp)

qt_add_executable(${PROJECT_NAME} ${EXECUTABLES})

target_link_libraries(${PROJECT_NAME} PRIVATE zip semver Qt6::Core Qt6::Quick Qt6::Qml Qt6::Network)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Get all SVG icons
file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../assets/icons/*.svg")

# Get all QML files
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../qml/*.qml")

# Set alias for each QML file
foreach(qml_file ${QML_FILES})
    get_filename_component(file_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${file_name})
endforeach()

set(QML_SOURCES
        ModLoader.cpp
        ModLoader.hpp
        ModSortFilterModel.cpp
        ModSortFilterModel.hpp
        ModStore.cpp
        ModStore.hpp)

qt_add_qml_module(${PROJECT_NAME}
        URI vsmodchecker
        VERSION 1.0
        QML_FILES ${QML_FILES}
        RESOURCES ${ICON_FILES} ../assets/logo/VSMM.png
        SOURCES ${QML_SOURCES})
