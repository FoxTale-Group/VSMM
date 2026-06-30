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

#include "Config.hpp"

#include <QDir>
#include <QStandardPaths>

namespace vsmodchecker {
Config::Config() {
    QDir().mkdir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    mConfigFile.setFileName(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() +
                            CONFIG_FILE_NAME.toString());

    if (mConfigFile.exists() && mConfigFile.open(QIODevice::ReadOnly)) {
        mConfig = QJsonDocument::fromJson(mConfigFile.readAll()).object();
        mConfigFile.close();
    } else {
        mConfig["vsmm"] = createDefaultConfig();
    }

    parseConfig();
}

Config::~Config() {
    if (mConfigFile.open(QIODevice::WriteOnly)) {
        mConfigFile.write(QJsonDocument(mConfig).toJson());
        qDebug() << "Config file saved";
        return;
    }

    qWarning() << "Config could not be saved";
}

const QDir &Config::getGameDir() const { return mGameDir; }
const QDir &Config::getModsDir() const { return mModsDir; }
bool Config::getDeleteOldModVersion() const { return mDeleteOldModVersion; }
QUrl Config::getGameDirQml() const { return getGameDir().absolutePath(); }
QUrl Config::getModsDirQml() const { return getModsDir().absolutePath(); }

void Config::setGameDirQml(const QUrl &dir) {
    mGameDir = dir.toString();
    emit gameDirChanged();
}

void Config::setModsDirQml(const QUrl &dir) {
    mModsDir = dir.toString();
    emit modsDirChanged();
}

void Config::setDeleteOldModVersion(bool deleteOldModVersion) {
    mDeleteOldModVersion = deleteOldModVersion;
    emit deletedOldModVersionChanged();
}

void Config::parseConfig() {
    static const QString defModsDir = QDir::cleanPath(
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/VintagestoryData/Mods");

    if (!mConfig["vsmm"].isObject()) {
        qWarning() << "Config file is missing vsmm object";
        return;
    }

    QJsonObject vsmm = mConfig["vsmm"].toObject();

    mGameDir = vsmm["gameDir"].toString();
    mModsDir = vsmm["modsDir"].toString(defModsDir);
    if (!mModsDir.exists()) {
        mModsDir = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                                   "/.var/app/at.vintagestory.VintageStory/config/VintagestoryData/Mods");

        if (!mModsDir.exists()) {
            mModsDir = QDir();
        }
    }
    mDeleteOldModVersion = vsmm["deleteOldModVersion"].toBool(true);

    emit gameDirChanged();
    emit modsDirChanged();
    emit deletedOldModVersionChanged();
}

QJsonObject Config::createDefaultConfig() {
    auto vsmm = QJsonObject();
    vsmm["gameDir"] = "";
    vsmm["modsDir"] = "";
    vsmm["deleteOldModVersion"] = true;

    return vsmm;
}
} // namespace vsmodchecker