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
#include <QIcon>

#include "ModListModel.hpp"
#include "ModManager.hpp"

namespace vsmodchecker {
    App::App(int &argc, char *argv[]) :
        QGuiApplication{argc, argv},
        mQmlEngine{this}, mNetworkManager{this}, mNetworkDiskCache{this}
    {
        setApplicationDisplayName(APP_DISPLAY_NAME);
        setApplicationName(APP_DISPLAY_NAME);
        setApplicationVersion(APP_VERSION);
        setWindowIcon(QIcon(":/qt/qml/vsmodchecker/icons/VSMM_Icon.png"));

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

        mNetworkDiskCache.setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        mNetworkManager.setCache(&mNetworkDiskCache);

        initQmlEngine(modsDir.toStdString());
    }

    void App::initQmlEngine(std::filesystem::path modsPath) {
        mQmlEngine.loadFromModule("vsmodchecker", "Main");
        mModImageProvider = new ModImageProvider();
        mQmlEngine.addImageProvider("modicon", mModImageProvider);

        auto modList = mQmlEngine.singletonInstance<ModListModel *>("vsmodchecker", "ModListModel");
        auto modManager = mQmlEngine.singletonInstance<ModManager *>("vsmodchecker", "ModManager");
        modManager->setNetworkManager(&mNetworkManager);
        modManager->setModImageProvider(mModImageProvider);
        modList->setModImageProvider(mModImageProvider);

        connect(modManager, &ModManager::modEntryAdded, modList, &ModListModel::modEntryAdded);
        connect(modManager, &ModManager::modsCleared, modList, &ModListModel::modsCleared);
        connect(modManager, &ModManager::modsCleared, mModImageProvider, &ModImageProvider::modsCleared);
        connect(modManager, &ModManager::modEntryUpdated, modList, &ModListModel::modEntryUpdated);
        connect(modManager, &ModManager::thumbnailReady, modList, &ModListModel::modEntryIconUpdated);

        modManager->setModsPath(std::move(modsPath));
        if (!modManager->initModsList()) {
            qFatal() << "Failed to initialize mods list";
        }
    }
} // vsmodchecker