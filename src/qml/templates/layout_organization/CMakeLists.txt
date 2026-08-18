project(LayoutOrganization VERSION 1.0)

qt_add_qml_module(${PROJECT_NAME}
        STATIC
        URI VSMM.${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        QML_FILES
            HorizontalDivider.qml
            LayoutHorizontalDivider.qml
            LayoutHorizontalSpacer.qml
            LayoutVerticalDivider.qml
            LayoutVerticalSpacer.qml
        IMPORTS VSMM.Theme)

# build order only, a qml-only module has no symbols to link; the executable links both,
# which is what registers their resources
add_dependencies(${PROJECT_NAME} Theme)
