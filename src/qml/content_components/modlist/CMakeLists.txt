project(ModList VERSION 1.0)

qt_add_qml_module(${PROJECT_NAME}
        STATIC
        URI VSMM.${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        QML_FILES
            ModEntry.qml
            ModEntry_ButtonsSection.qml
            ModEntry_Icon.qml
            ModList.qml
        IMPORTS VSMM.Theme VSMM.LayoutOrganization VSMM.ModStore VSMM.ModSortFilterModel
        IMPORT_PATH ${VSMM_QML_STYLES_PATH})

add_dependencies(${PROJECT_NAME} Theme LayoutOrganization ModStore ModSortFilterModel)
