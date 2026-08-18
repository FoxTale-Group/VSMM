project(ContentComponents VERSION 1.0)

qt_add_qml_module(${PROJECT_NAME}
        STATIC
        URI VSMM.${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        QML_FILES
            MainButtonsSection.qml
            ModSearchBar.qml
            StatCards.qml
        IMPORTS VSMM.Theme VSMM.ModStore VSMM.ModSortFilterModel
        IMPORT_PATH ${VSMM_QML_STYLES_PATH})

add_dependencies(${PROJECT_NAME} Theme ModStore ModSortFilterModel)
