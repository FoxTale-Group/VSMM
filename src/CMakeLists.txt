
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

configure_file(config.hpp.in config.hpp)

qt_add_executable(${PROJECT_NAME}
        main.cpp
        App.cpp
        ZipArchive.cpp
        ZipArchive.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE zip cpr semver Qt6::Core Qt6::Quick Qt6::Qml)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(../qml/Main.qml PROPERTIES
        QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(${PROJECT_NAME}
        URI main
        VERSION 1.0
        QML_FILES ../qml/Main.qml
        RESOURCES
            ../icons/add.svg
            ../icons/add_box.svg
            ../icons/delete.svg
            ../icons/download.svg
            ../icons/download_all.svg
            ../icons/sync.svg
            ../icons/upgrade_one.svg
            ../icons/close.svg
            ../icons/close_fullscreen.svg
            ../icons/fullscreen.svg
            ../icons/minimize.svg
            ../icons/select_check_box.svg
            ../icons/check_update.svg
            ../icons/extension.svg
        SOURCES
            ModListModel.cpp
            ModListModel.hpp
            ModManager.cpp
            ModManager.hpp


)