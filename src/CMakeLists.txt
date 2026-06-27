qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

configure_file(config.hpp.in config.hpp)

set(EXECUTABLES
        main.cpp
        App.cpp
        ZipArchive.cpp
        ModEntry.cpp
        ModImageProvider.cpp
        ModListModel.cpp)

qt_add_executable(${PROJECT_NAME} ${EXECUTABLES})

target_link_libraries(${PROJECT_NAME} PRIVATE zip semver Qt6::Core Qt6::Quick Qt6::Qml Qt6::Network)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Get all SVG icons
file(GLOB ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../assets/icons/*.svg")

# Get all QML files
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "../qml/*.qml")

# Set alias for each QML file
foreach(qml_file ${QML_FILES})
    get_filename_component(file_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${file_name})
endforeach()

set(QML_SOURCES
        ModLoader.cpp
        ModLoader.hpp
        ModSortFilterModel.cpp
        ModSortFilterModel.hpp
        ModStore.cpp
        ModStore.hpp)

qt_add_qml_module(${PROJECT_NAME}
        URI vsmodchecker
        VERSION 1.0
        QML_FILES ${QML_FILES}
        RESOURCES ${ICON_FILES} ../assets/logo/VSMM.png
        SOURCES ${QML_SOURCES})

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