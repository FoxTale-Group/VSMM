project(Theme VERSION 1.0)

# the app icons live here because Theme is their only consumer, and the aliases pin the
# resource paths instead of relying on Qt stripping the leading "../"
file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../../assets/icons/*.svg")
foreach (icon ${ICON_FILES})
    get_filename_component(icon_name ${icon} NAME)
    set_source_files_properties(${icon} PROPERTIES QT_RESOURCE_ALIAS assets/icons/${icon_name})
endforeach ()
set_source_files_properties(../../assets/logo/VSMM.png PROPERTIES QT_RESOURCE_ALIAS assets/logo/VSMM.png)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME}
        STATIC
        URI VSMM.${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        QML_FILES Theme.qml
        RESOURCES ${ICON_FILES} ../../assets/logo/VSMM.png)
