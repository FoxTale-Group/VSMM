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

#include "GameMngr.hpp"
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(cGameMngr, "gamemngr");

using namespace Qt::StringLiterals;

namespace vsmm {
void GameMngr::setConfig(Config *config) {
    mConfig = config;
    connect(mConfig, &Config::gameConfigPathChanged, this, &GameMngr::parseClientCfg);
    readGameVersion();
}
const QList<QDir> &GameMngr::getModsDirs() const { return mModsDirs; }

const semver::version &GameMngr::getGameVersion() const { return mGameVersion; }

void GameMngr::launchGame() {
    if (mConfig->getPath(CONFIG_GAMEEXE_JSON_KEY).isEmpty()) {
        qCCritical(cGameMngr) << u"config paths.%1 value is empty"_s.arg(CONFIG_GAMEEXE_JSON_KEY);
        return;
    }

    // Clear arguments
    mGameProcess.setArguments({});

    qint64 pid{-1};
    if (!mGameProcess.startDetached(&pid)) {
        qCCritical(cGameMngr, "Failed to start game exe");
        return;
    }

    qCDebug(cGameMngr) << u"Process started as %1"_s.arg(pid);
}

void GameMngr::parseClientCfg() {
    mModsDirs.clear();

    const QString clientSettingsFilename = "clientsettings.json";

    if (!mConfig) {
        qCFatal(cGameMngr, "Config not set");
    }

    QDir configGameDir = mConfig->getPath(CONFIG_GAMEDIR_JSON_KEY);
    if (!configGameDir.exists(clientSettingsFilename)) {
        qCCritical(cGameMngr, "Client settings file does not exist");
        emit modsDirsChanged();
        return;
    }

    QFile clientSettingsFile{configGameDir.absolutePath() + QDir::separator() + clientSettingsFilename};
    if (!clientSettingsFile.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::ExistingOnly)) {
        qCCritical(cGameMngr) << u"Failed to open client settings file"_s.arg(clientSettingsFile.fileName());
        emit modsDirsChanged();
        return;
    }

    QJsonDocument clientSettingsDoc = QJsonDocument::fromJson(clientSettingsFile.readAll());
    if (!clientSettingsDoc.isObject()) {
        qCCritical(cGameMngr, "Client settings file is not a valid JSON object");
        emit modsDirsChanged();
        return;
    }

    auto clientSettings = clientSettingsDoc.object();
    if (const auto &[valid, reason] = checkClientSettingsVer(clientSettings); !valid) {
        qCCritical(cGameMngr) << u"Detected unsupported clientsettings: %1"_s.arg(reason);
        emit modsDirsChanged();
    }

    readModsPaths(clientSettings);
    emit modsDirsChanged();
}

void GameMngr::readGameVersion() {
    if (mConfig->getPath(CONFIG_GAMEEXE_JSON_KEY).isEmpty()) {
        qCCritical(cGameMngr) << u"config paths.%1 value is empty"_s.arg(CONFIG_GAMEEXE_JSON_KEY);
        return;
    }

    QFileInfo gameExe{mConfig->getPath(CONFIG_GAMEEXE_JSON_KEY)};

    mGameProcess.setProgram(gameExe.absoluteFilePath());
    mGameProcess.setWorkingDirectory(gameExe.absolutePath());
    mGameProcess.setArguments({"--version"});

    mGameProcess.start();
    if (!mGameProcess.waitForFinished()) {
        qCCritical(cGameMngr, "Failed to start game exe");
        return;
    }

    mGameVersion = semver::version::parse(mGameProcess.readAllStandardOutput().trimmed().toStdString());
    qCDebug(cGameMngr) << u"Game version detected: %1"_s.arg(mGameVersion.str());
}

void GameMngr::readModsPaths(const QJsonObject &clientSettings) {
    using namespace Qt::StringLiterals;
    if (!clientSettings["stringListSettings"_L1].isObject()) {
        qCCritical(cGameMngr, "Failed to locate stringListSettings in client settings file");
        return;
    }

    auto stringListSettings = clientSettings["stringListSettings"_L1].toObject();
    if (!stringListSettings["modPaths"_L1].isArray()) {
        qCCritical(cGameMngr, "Invalid modPaths");
        return;
    }

    for (const auto &path : stringListSettings["modPaths"_L1].toArray()) {
        if (path.toStringView() == "Mods"_L1 || path.isNull() || path.isUndefined() || path.toStringView().isEmpty()) {
            continue;
        }

        mModsDirs.append(path.toString());
    }
}

QPair<bool, QString> GameMngr::checkClientSettingsVer(const QJsonObject &clientSettings) const {
    if (!clientSettings["stringSettings"_L1].isObject()) {
        return {false, u"Invalid stringSettings"_s};
    }

    auto stringSettings = clientSettings["stringSettings"_L1].toObject();
    if (auto clSettingsVersion = stringSettings["settingsVersion"_L1].toStringView();
        !CLIENT_SETTINGS_VER_SUPPORT.contains(clSettingsVersion)) {
        return {false, u"Unsupported clientsettings version %1"_s.arg(clSettingsVersion)};
    }

    return {true, {}};
}
} // namespace vsmm