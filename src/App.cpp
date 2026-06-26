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
#include "ModSortFilterModel.hpp"
#include "ModLoader.hpp"

namespace vsmodchecker {
    App::App(int &argc, char *argv[]) :
        QGuiApplication{argc, argv},
        mNetworkManager{this}, mNetworkDiskCache{this}, mQmlEngine{this}
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
            modsDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                QDir::separator() + "VintagestoryData" + QDir::separator() + "Mods";
        }

        connect(&mQmlEngine, &QQmlApplicationEngine::objectCreationFailed, [](const QUrl &url) {
            qFatal() << QString("QML object creation failed %1").arg(url.toString());
        });

        mNetworkDiskCache.setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        mNetworkManager.setCache(&mNetworkDiskCache);

        initQmlEngine(modsDir);
    }

    void App::initQmlEngine(const QString &modsPath) {
        mQmlEngine.loadFromModule("vsmodchecker", "Main");
        mModImageProvider = new ModImageProvider();
        mQmlEngine.addImageProvider("modicon", mModImageProvider);

        auto modManager = mQmlEngine.singletonInstance<ModLoader *>("vsmodchecker", "ModLoader");
        auto modSortFilterModel = mQmlEngine.singletonInstance<ModSortFilterModel *>("vsmodchecker", "ModSortFilterModel");
        auto modStore = mQmlEngine.singletonInstance<ModStore *>("vsmodchecker", "ModStore");
        auto modListModel = mQmlEngine.singletonInstance<ModListModel *>("vsmodchecker", "ModListModel");
        modSortFilterModel->setSourceModel(modListModel);

        modManager->setNetworkManager(&mNetworkManager);
        modManager->setStore(modStore);

        modListModel->setStore(modStore);
        modListModel->setModImageProvider(mModImageProvider);

        connect(modManager, &ModLoader::modIconDownloaded, mModImageProvider, &ModImageProvider::onImageReceived);
        connect(mModImageProvider, &ModImageProvider::imageAdded, modListModel, &ModListModel::iconUpdate);
        connect(modStore, &ModStore::modsReloading, mModImageProvider, &ModImageProvider::onModsReloading);

        if (!modManager->setModsPath(modsPath)) {
            qWarning() << "Mods directory not found; starting with an empty mod list";
        } else if (!modManager->initModsList()) {
            qWarning() << "Failed to initialize mods list";
        }
    }
} // vsmodchecker