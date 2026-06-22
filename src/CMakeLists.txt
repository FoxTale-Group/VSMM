project(VSModChecker)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_add_executable(${PROJECT_NAME} main.cpp
        ZipArchive.cpp
        ZipArchive.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE zip cpr semver Qt6::Core Qt6::Quick Qt6::Qml)

set_source_files_properties(../qml/Main.qml PROPERTIES
        QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(${PROJECT_NAME}
        URI main
        VERSION 1.0
        QML_FILES ../qml/Main.qml
)