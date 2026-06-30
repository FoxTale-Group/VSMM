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
        mConfig["vsmm"] = QJsonObject();
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

QAnyStringView Config::getGameDir() const { return mGameDir; }
QAnyStringView Config::getModsDir() const { return mModsDir; }
bool Config::getDeleteOldModVersion() const { return mDeleteOldModVersion; }
QUrl Config::getGameDirQml() const { return getGameDir().toString(); }
QUrl Config::getModsDirQml() const { return getModsDir().toString(); }

void Config::setGameDirQml(const QUrl &dir) { updateConfig(ConfigKeys::GameDir, dir.toString()); }
void Config::setModsDirQml(const QUrl &dir) { updateConfig(ConfigKeys::ModsDir, dir.toString()); }
void Config::setDeleteOldModVersion(bool deleteOldModVersion) {
    updateConfig(ConfigKeys::DeleteOldModVersion, deleteOldModVersion);
}

void Config::updateConfig(ConfigKeys key, QVariant &&value) {
    auto vsmmObj = mConfig["vsmm"].toObject();

    switch (key) {
    case ConfigKeys::ModsDir:
        vsmmObj["modsDir"] = mModsDir = value.toString();
        emit modsDirChanged();
        break;
    case ConfigKeys::DeleteOldModVersion:
        vsmmObj["deleteOldModVersion"] = mDeleteOldModVersion = value.toBool();
        emit deletedOldModVersionChanged();
        break;
    case ConfigKeys::GameDir:
        vsmmObj["gameDir"] = mGameDir = value.toString();
        emit gameDirChanged();
        break;
    }
    mConfig["vsmm"] = std::move(vsmmObj);
}

void Config::parseConfig() {
    static const QString defModsDir = QDir::cleanPath(
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/VintagestoryData/Mods");

    if (!mConfig["vsmm"].isObject()) {
        qWarning() << "Config file is missing vsmm object";
        return;
    }

    QJsonObject vsmm = mConfig["vsmm"].toObject();
    QString gameDir = vsmm["gameDir"].toString();
    QString modsDir = vsmm["modsDir"].toString(defModsDir);
    if (!QDir{}.exists(modsDir)) {
        modsDir = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                                  "/.var/app/at.vintagestory.VintageStory/config/VintagestoryData/Mods");

        qDebug() << "Checking flatpak version...";
        if (!QDir{}.exists(modsDir)) {
            modsDir.clear();
            qWarning() << "Mods dir does not exist";
        }
    }

    updateConfig(ConfigKeys::ModsDir, QVariant::fromValue(std::move(modsDir)));
    updateConfig(ConfigKeys::GameDir, QVariant::fromValue(std::move(modsDir)));
    updateConfig(ConfigKeys::DeleteOldModVersion, vsmm["deleteOldModVersion"].toBool(true));
}

} // namespace vsmodchecker