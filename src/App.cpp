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
#include <qqmlcontext.h>
#include <QStandardPaths>

#include "ModListModel.hpp"

namespace vsmodchecker {
    App::App(int &argc, char *argv[]) :
        QGuiApplication{argc, argv},
        mQmlEngine{this}, mNetworkManager{this}
    {
        setApplicationDisplayName(APP_DISPLAY_NAME);
        setApplicationName(APP_DISPLAY_NAME);
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

        connect(&mQmlEngine, &QQmlApplicationEngine::objectCreationFailed, [](const QUrl &url) {
            qFatal() << QString("QML object creation failed %1").arg(url.toString());
        });

        initQmlEngine(modsDir.toStdString());
    }

    void App::initQmlEngine(std::filesystem::path modsPath) {
        mQmlEngine.loadFromModule("vsmodchecker", "Main");
        if (mQmlEngine.rootObjects().isEmpty()) {
            throw std::runtime_error("Failed to load QML");
        }

        auto modList = mQmlEngine.singletonInstance<ModListModel *>("vsmodchecker", "ModListModel");
        auto modManager = mQmlEngine.singletonInstance<ModManager *>("vsmodchecker", "ModManager");
        modManager->setNetworkManager(&mNetworkManager);
        connect(modManager, &ModManager::modEntryAdded, modList, &ModListModel::modEntryAdded);
        connect(modManager, &ModManager::modsCleared, modList, &ModListModel::modsCleared);
        connect(modManager, &ModManager::modEntryUpdated, modList, &ModListModel::modEntryUpdated);

        modManager->setModsPath(std::move(modsPath));
        if (!modManager->initModsList()) {
            throw std::runtime_error("Failed to initialize mods list");
        }
    }
} // vsmodchecker