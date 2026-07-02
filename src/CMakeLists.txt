qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0005 NEW)

configure_file(constants.hpp.in constants.hpp)

add_subdirectory(config Config)
add_subdirectory(zip)
add_subdirectory(modentry)
add_subdirectory(modstore ModStore)
add_subdirectory(imgprovider)
add_subdirectory(modlistmodel ModListModel)
add_subdirectory(modloader ModLoader)
add_subdirectory(modsortfiltermodel ModSortFilterModel)

set(EXECUTABLES
        main.cpp
        App.cpp)

qt_add_executable(${PROJECT_NAME} ${EXECUTABLES})
add_dependencies(${PROJECT_NAME} Config ZipArchive ModStore ImgProvider ModListModel ModLoader ModSortFilterModel)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Quick Qt6::Qml Qt6::Network Config ZipArchive ModStore ImgProvider ModListModel ModLoader ModSortFilterModel)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Get all SVG icons
file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../assets/icons/*.svg")

# Get all QML and JS files
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "qml/*.qml" "qml/js/*.js")

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
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# Install Linux-specific assets required for AppImage, DEB, and RPM
if (UNIX AND NOT APPLE)
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
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VSMM.desktop"
            DESTINATION share/applications)

    # Install the icon
    install(FILES "${CMAKE_SOURCE_DIR}/assets/logo/VSMM.png"
            DESTINATION share/icons/hicolor/256x256/apps)
endif ()

# CPACK METADATA
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${VSMM_RELEASE_VERSION}")
set(CPACK_PACKAGE_VENDOR "FoxTale Group - VSMM Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A custom mod manager for Vintage Story")
set(CPACK_PACKAGE_CONTACT "https://github.com/FoxTale-Group/VSMM")

set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/pkg")
set(CPACK_THREADS 6)
set(CPACK_COMPRESSION_LEVEL 9)

# OS-SPECIFIC GENERATORS
if (UNIX AND NOT APPLE)
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

    #[[elseif(WIN32)
        # NSIS for install wizard, ZIP for portable standalone package
        set(CPACK_GENERATOR "NSIS;ZIP")
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
        set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/assets/logo/VSMM_Logo.bmp")]]
endif ()

include(CPack)
