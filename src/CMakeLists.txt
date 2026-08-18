# Setup Qt Policies
qt_policy(SET QTP0001 NEW) # Set default path to qrc:/qt/qml
qt_policy(SET QTP0004 NEW) # Generates separate qml dirs for each module
qt_policy(SET QTP0005 NEW) # QML Module IMPORTS can use CMake target names

configure_file(constants.hpp.in constants.hpp)

# Set unified target directory for sub-libs generated qmldir and .qmltypes
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml_imports)

# VSMMStyle is a plain qrc directory, not a QML module, so every module that imports it
# needs this on its IMPORT_PATH for qmlls and qmllint to resolve the types
set(VSMM_QML_STYLES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/qml/styles)

# Point Qt Creator's code model at the generated modules, plus the VSMMStyle style sources
# so `import VSMMStyle` resolves in the editor (the style otherwise only ever exists as a
# qrc blob, invisible to tooling).
#
# Rebuilt from scratch on every configure, NOT appended to: `list(APPEND)` reads back the
# FORCE-cached value, so each configure grew the list by another copy of the same path and
# the code model re-scanned (and re-queued plugin type-info reads for) the same tree N times.
#
# STRING, not PATH -- this is a ";"-separated list; PATH marks it as a single directory.
set(QML_IMPORT_PATH
        "${QT_QML_OUTPUT_DIRECTORY};${VSMM_QML_STYLES_PATH}"
        CACHE STRING "Qt Creator QML Import Paths" FORCE)

# Add libs and qml modules subdirectories
add_subdirectory(interfaces Interfaces)
add_subdirectory(httpclient)
add_subdirectory(config Config)
add_subdirectory(zip)
add_subdirectory(modentry)
add_subdirectory(modstore ModStore)
add_subdirectory(imgprovider)
add_subdirectory(modlistmodel ModListModel)
add_subdirectory(modloader ModLoader)
add_subdirectory(modsortfiltermodel ModSortFilterModel)
add_subdirectory(gamemngr GameMngr)

# QML-only modules, one per qml subdirectory; the second argument keeps their binary dirs
# apart because each calls project(<Name>)
add_subdirectory(qml Theme)
add_subdirectory(qml/js Helpers)
add_subdirectory(qml/animations Animations)
add_subdirectory(qml/templates/layout_organization LayoutOrganization)
add_subdirectory(qml/templates/window_components WindowComponents)
add_subdirectory(qml/templates Templates)
add_subdirectory(qml/content_components/modlist ModList)
add_subdirectory(qml/content_components ContentComponents)
add_subdirectory(qml/content_components/settings SettingsTabs)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

# Set main app executable sources
if (UNIX AND NOT APPLE)
    set(EXECUTABLES main.cpp App.cpp)
    qt_add_executable(${PROJECT_NAME} ${EXECUTABLES})
elseif (WIN32)
    set(EXECUTABLES main.cpp App.cpp app.rc)
    qt_add_executable(${PROJECT_NAME} WIN32 ${EXECUTABLES})
endif ()


# Set and add project dependencies & link target libraries
set(PROJECT_DEPENDENCIES_AND_LIBRARIES
        Config
        ZipArchive
        ModStore
        ImgProvider
        ModListModel
        ModLoader
        ModSortFilterModel
        HttpClient
        GameMngr
        Theme
        Helpers
        Animations
        LayoutOrganization
        WindowComponents
        Templates
        ModList
        ContentComponents
        SettingsTabs)
add_dependencies(${PROJECT_NAME} ${PROJECT_DEPENDENCIES_AND_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_DEPENDENCIES_AND_LIBRARIES} Qt6::QuickControls2)

# Include target dir
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(${PROJECT_NAME}
        URI VSMM.App
        VERSION 1.0
        QML_FILES
            qml/Main.qml
            qml/Settings.qml
            qml/AddMod.qml
        IMPORT_PATH ${VSMM_QML_STYLES_PATH}
        # full URIs, not target names; QTP0005 does not translate IMPORTS entries
        IMPORTS
            VSMM.Theme
            VSMM.Helpers
            VSMM.Templates
            VSMM.LayoutOrganization
            VSMM.ModList
            VSMM.ContentComponents
            VSMM.SettingsTabs
            VSMM.Config
            VSMM.GameMngr
            VSMM.ModStore)

# Custom Qt Quick Controls style "VSMMStyle", resolved at runtime by QQuickStyle::setStyle("VSMMStyle").
# It is a plain style directory (qmldir + control impls) placed on the default qrc import
# path at qt/qml/VSMMStyle/, NOT a qt_add_qml_module type module. BASE strips the source prefix
# so the files land directly under the module path.
qt_add_resources(${PROJECT_NAME} "VSMMStyle"
        PREFIX "/qt/qml/VSMMStyle"
        BASE "qml/styles/VSMMStyle"
        FILES
            qml/styles/VSMMStyle/qmldir
            qml/styles/VSMMStyle/Button.qml
            qml/styles/VSMMStyle/CheckBox.qml
            qml/styles/VSMMStyle/ComboBox.qml
            qml/styles/VSMMStyle/Label.qml
            qml/styles/VSMMStyle/RadioButton.qml
            qml/styles/VSMMStyle/ScrollBar.qml
            qml/styles/VSMMStyle/Switch.qml
            qml/styles/VSMMStyle/TabBar.qml
            qml/styles/VSMMStyle/TabButton.qml
            qml/styles/VSMMStyle/TextField.qml
            qml/styles/VSMMStyle/ToolTip.qml
)

# -- CPACK --

# INSTALL RULES

# Install the main executable
if (UNIX AND NOT APPLE)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
elseif (WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    # Bundle the libzip to NSIS installer
    install(FILES "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>" DESTINATION bin)
endif ()

# CPACK METADATA
set(CPACK_PACKAGE_NAME "vintagestory-mod-manager")
set(CPACK_PACKAGE_VERSION "${VSMM_RELEASE_VERSION}")
set(CPACK_PACKAGE_VENDOR "FoxTale Group - VSMM Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A custom mod manager for Vintage Story")
set(CPACK_PACKAGE_CONTACT "https://github.com/FoxTale-Group/VSMM")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/pkg")
set(CPACK_THREADS 6)
set(CPACK_COMPRESSION_LEVEL 9)

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
# OS-SPECIFIC GENERATORS AND METADATA
if (UNIX AND NOT APPLE)
    # Install Linux-specific assets required for AppImage, DEB, and RPM
    # Dynamically generate .desktop file
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VSMM.desktop"
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=${APP_DISPLAY_NAME}\n"
            "Comment=A custom mod manager for Vintage Story\n"
            "Exec=${CMAKE_PROJECT_NAME}\n"
            "Icon=VSMM\n"
            "Categories=Game;Utility;\n"
            "Terminal=false\n"
    )

    # Install .desktop file
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VSMM.desktop" DESTINATION share/applications)

    # Install the icon
    install(FILES "${CMAKE_SOURCE_DIR}/assets/logo/VSMM.png" DESTINATION share/icons/hicolor/256x256/apps)

    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

    set(CPACK_GENERATOR "TXZ;TGZ;DEB;RPM")

    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "qml6-module-qtquick-dialogs,
            qml6-module-qtquick-effects,
            qml6-module-qtquick-window,
            libqt6network6,
            qt6-qpa-plugins,
            libzip5")

    # Rell the DEB/RPM packagers to automatically find library dependencies
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_RPM_PACKAGE_AUTOREQPROV "yes")
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
            "/usr/share/applications"
            "/usr/share/icons"
            "/usr/share/icons/hicolor"
            "/usr/share/icons/hicolor/256x256"
            "/usr/share/icons/hicolor/256x256/apps")

    # Version formatting
    string(REPLACE "-" "~" DISTRO_VER "${VSMM_RELEASE_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${DISTRO_VER}")
    set(CPACK_RPM_PACKAGE_VERSION "${DISTRO_VER}")

    # Get absolute path to logo and set package icon
    get_filename_component(ABS_ICON_PATH "${CMAKE_SOURCE_DIR}/assets/logo/VSMM.png" ABSOLUTE)
    set(CPACK_PACKAGE_ICON "${ABS_ICON_PATH}")

    #set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/assets/logo/VSMM.png")
    #set(CPACK_PACKAGE_ICON "")

elseif(WIN32)
    # NSIS for install wizard, ZIP for portable standalone package
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")

    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win64-installer")
    set(CPACK_ARCHIVE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win64-portable")

    # Set image shown inside installer window
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/assets/logo/VSMM_Logo.bmp")

    # Set the installer UI and paths
    set(CPACK_NSIS_PACKAGE_NAME "Vintage Story Mod Manager")
    set(CPACK_NSIS_DISPLAY_NAME "Vintage Story Mod Manager")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vintage Story Mod Manager")

    # Setup license file to be included in NSIS installer
    configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")

    # Set installer icon
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/logo/VSMM_Logo.ico")

    # Setup NSIS installer start menu, desktop and control panel shortcut
    set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "Vintage Story Mod Manager")
    set(CPACK_CREATE_DESKTOP_LINKS "${PROJECT_NAME}")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${PROJECT_NAME}.exe")
    # Create shortcut on user desktop
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
            "CreateShortCut \\\"$DESKTOP\\\\Vintage Story Mod Manager.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe\\\"")
    # Delete desktop shortcut when user uninstalls program
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
            "Delete \\\"$DESKTOP\\\\Vintage Story Mod Manager.lnk\\\"")

    # Deployment script
    qt_generate_deploy_qml_app_script(
            TARGET ${PROJECT_NAME}
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif ()

include(CPack)