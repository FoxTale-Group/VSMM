// Never built or shipped. This file exists only so linuxdeploy-plugin-qt's qmlimportscanner
// bundles QML modules that VSMM resolves at runtime rather than through a QML import, which
// static scanning cannot discover.
//
// QtQuick.Controls.Basic: App.cpp calls QQuickStyle::setFallbackStyle("Basic"). VSMMStyle
// implements 12 controls; anything else falls back to Basic, and no .qml file imports it.
//
// Add an import here whenever a module is reached from C++ instead of from QML.

import QtQuick
import QtQuick.Controls.Basic

Item {}
