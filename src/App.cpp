/*
 * VS Mod Manager - A mod management tool for Vintage Story
 * Copyright (C) 2026 Amaroq & StardustVulpine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "App.hpp"
#include <config.hpp>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStandardPaths>

namespace vsmodchecker {
    App::App(int &argc, char *argv[]) :
        QGuiApplication{argc, argv},
        mQmlEngine{this}, mNetworkManager{this}, mModManager{mNetworkManager, this}
    {
        setApplicationDisplayName(APP_NAME);
        setApplicationName(APP_NAME);
        setApplicationVersion(APP_VERSION);

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption modsDirOption("mods-dir", "Path to mods directory", "path", QString());
        parser.addOption(modsDirOption);

        parser.process(*this);

        QString modsDir = parser.value(modsDirOption);
        if (modsDir.isEmpty()) {
            modsDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/VintagestoryData/Mods";
        }
        if (!mModManager.initModsList(modsDir.toStdString())) {
            throw std::runtime_error("Failed to initialize mods list");
        }

        mQmlEngine.loadFromModule("main", "Main");
        if (mQmlEngine.rootObjects().isEmpty()) {
            throw std::runtime_error("Failed to load QML");
        }

        QObject *rootObj = mQmlEngine.rootObjects().first();
        auto listModel = rootObj->findChild<QObject*>("modModel");

        connect(&mModManager, &ModManager::modAdded, this, [listModel](const ModManager::ModEntry& mod) {
            QVariantMap entry;
            entry["modName"] = mod.name;
            entry["author"] = mod.author;
            entry["version"] = mod.version;
            entry["updateVersion"] = mod.latestVersion;
            entry["modEnabled"] = true;
            entry["tag"]= "Farming";

            QMetaObject::invokeMethod(listModel, "appendEntry",
                                  Q_ARG(QVariant, entry));
        });
    }
} // vsmodchecker