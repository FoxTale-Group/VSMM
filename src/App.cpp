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

#if defined(Q_OS_LINUX)
    mQmlEngine.rootContext()->setContextProperty("IS_LINUX", true);
    mQmlEngine.rootContext()->setContextProperty("IS_WINDOWS", false);
    mQmlEngine.rootContext()->setContextProperty("IS_MACOS", false);
#elif defined(Q_OS_WINDOWS)
    mQmlEngine.rootContext()->setContextProperty("IS_LINUX", false);
    mQmlEngine.rootContext()->setContextProperty("IS_WINDOWS", true);
    mQmlEngine.rootContext()->setContextProperty("IS_MACOS", false);
#elif defined(Q_OS_MACOS)
    mQmlEngine.rootContext()->setContextProperty("IS_LINUX", false);
    mQmlEngine.rootContext()->setContextProperty("IS_WINDOWS", false);
    mQmlEngine.rootContext()->setContextProperty("IS_MACOS", true);
#endif

    auto config = mQmlEngine.singletonInstance<Config *>("vsmm", "Config");
    auto gameMngr = mQmlEngine.singletonInstance<GameMngr *>("vsmm", "GameMngr");
    auto modLoader = mQmlEngine.singletonInstance<ModLoader *>("vsmm", "ModLoader");
    auto modSortFilterModel = mQmlEngine.singletonInstance<ModSortFilterModel *>("vsmm", "ModSortFilterModel");
    auto modStore = mQmlEngine.singletonInstance<ModStore *>("vsmm", "ModStore");
    auto modListModel = mQmlEngine.singletonInstance<ModListModel *>("vsmm", "ModListModel");

    modListModel->setStore(modStore);
    modListModel->setModImageProvider(mModImageProvider);

    modSortFilterModel->setSourceModel(modListModel);

    gameMngr->setConfig(config);

    modStore->setConfig(config);
    modStore->setGameMngr(gameMngr);

    modLoader->setHttpClient(&mHttpClient);
    modLoader->setStore(modStore);
    modLoader->setGameMngr(gameMngr);

    connect(modLoader, &ModLoader::modIconDownloaded, mModImageProvider, &ModImageProvider::onImageReceived);
    connect(mModImageProvider, &ModImageProvider::imageAdded, modListModel, &ModListModel::iconUpdate);
    connect(modStore, &ModStore::modsReloading, mModImageProvider, &ModImageProvider::onModsReloading);

    config->validate();
}
} // namespace vsmm