# Setup Qt Policies
qt_policy(SET QTP0001 NEW) # Set default path to qrc:/qt/qml
qt_policy(SET QTP0004 NEW) # Generates separate qml dirs for each module
qt_policy(SET QTP0005 NEW) # QML Module IMPORTS can use CMake target names

configure_file(constants.hpp.in constants.hpp)

# Set unified target directory for sub-libs generated qmldir and .qmltypes
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml_imports)

# Point QML import path directly to output directory
list(APPEND QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY})
set(QML_IMPORT_PATH ${QML_IMPORT_PATH} CACHE PATH "Qt Creator QML Import Paths" FORCE)

# Add libs and qml modules subdirectories
add_subdirectory(httpclient)
add_subdirectory(config Config)
add_subdirectory(zip)
add_subdirectory(modentry)
add_subdirectory(modstore ModStore)
add_subdirectory(imgprovider)
add_subdirectory(modlistmodel ModListModel)
add_subdirectory(modloader ModLoader)
add_subdirectory(modsortfiltermodel ModSortFilterModel)

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
        HttpClient)
add_dependencies(${PROJECT_NAME} ${PROJECT_DEPENDENCIES_AND_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_DEPENDENCIES_AND_LIBRARIES} Qt6::QuickControls2)

# Include target dir
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Get all SVG icons
file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../assets/icons/*.svg")

# Get all QML and JS files
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "qml/*.qml" "qml/js/*.js")

# Setup qml/Theme.qml as QML Singleton type
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME}
        URI vsmm
        VERSION 1.0
        QML_FILES ${QML_FILES}
        RESOURCES ${ICON_FILES} ../assets/logo/VSMM.png
        IMPORTS
            Config
            ModStore
            ModListModel
            ModLoader
            ModSortFilterModel)

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