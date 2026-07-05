/*
 * VSMM - A mod management tool for Vintage Story
 * Copyright (C) 2026 FoxTale-Group VSMM Team
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
#include <Config.hpp>
#include <QCommandLineParser>
#include <QIcon>
#include <QStandardPaths>
#include <constants.hpp>
#include <qqmlcontext.h>

#include <ModListModel.hpp>
#include <ModLoader.hpp>
#include <ModSortFilterModel.hpp>

namespace vsmm {
App::App(int &argc, char *argv[]) : QGuiApplication{argc, argv} {
    setApplicationDisplayName(APP_DISPLAY_NAME);
    setApplicationName(APP_DISPLAY_NAME);
    setApplicationVersion(APP_VERSION);
    setWindowIcon(QIcon(":/qt/qml/vsmm/assets/logo/VSMM.png"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*this);

    connect(&mQmlEngine, &QQmlApplicationEngine::objectCreationFailed,
            [](const QUrl &url) { qFatal() << QString("QML object creation failed %1").arg(url.toString()); });

    initQmlEngine();
}

void App::initQmlEngine() {
    mModImageProvider = new ModImageProvider();
    mQmlEngine.addImageProvider("modicon", mModImageProvider);
    mQmlEngine.loadFromModule("vsmm", "Main");

    auto config = mQmlEngine.singletonInstance<Config *>("vsmm", "Config");
    auto modManager = mQmlEngine.singletonInstance<ModLoader *>("vsmm", "ModLoader");
    auto modSortFilterModel = mQmlEngine.singletonInstance<ModSortFilterModel *>("vsmm", "ModSortFilterModel");
    auto modStore = mQmlEngine.singletonInstance<ModStore *>("vsmm", "ModStore");
    auto modListModel = mQmlEngine.singletonInstance<ModListModel *>("vsmm", "ModListModel");
    modSortFilterModel->setSourceModel(modListModel);

    modStore->setConfig(config);

    modManager->setHttpClient(&mHttpClient);
    modManager->setStore(modStore);
    modManager->setConfig(config);

    modListModel->setStore(modStore);
    modListModel->setModImageProvider(mModImageProvider);

    connect(modManager, &ModLoader::modIconDownloaded, mModImageProvider, &ModImageProvider::onImageReceived);
    connect(mModImageProvider, &ModImageProvider::imageAdded, modListModel, &ModListModel::iconUpdate);
    connect(modStore, &ModStore::modsReloading, mModImageProvider, &ModImageProvider::onModsReloading);

    if (!modManager->initModsList()) {
        qWarning() << "Failed to initialize mods list";
    }
}
} // namespace vsmm