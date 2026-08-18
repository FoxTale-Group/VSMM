project(SettingsTabs VERSION 1.0)

qt_add_qml_module(${PROJECT_NAME}
        STATIC
        URI VSMM.${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        QML_FILES
            SettingsTab_Advanced.qml
            SettingsTab_Appearance.qml
            SettingsTab_Configuration.qml
            SettingsTab_General.qml
        IMPORTS VSMM.Theme VSMM.Templates VSMM.LayoutOrganization VSMM.Helpers VSMM.Config
        IMPORT_PATH ${VSMM_QML_STYLES_PATH})

add_dependencies(${PROJECT_NAME} Theme Templates LayoutOrganization Helpers Config)
