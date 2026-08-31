project(WindowComponents VERSION 1.0)

qt_add_qml_module(${PROJECT_NAME}
        STATIC
        URI VSMM.${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        QML_FILES
            WindowResizers.qml
            WindowTitleBar.qml
        IMPORTS VSMM.Theme)

add_dependencies(${PROJECT_NAME} Theme)
