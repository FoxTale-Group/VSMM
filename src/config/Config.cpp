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

#include "Config.hpp"

#include <QDir>
#include <QJsonArray>
#include <QStandardPaths>

namespace vsmm {
Config::Config() {
    QDir().mkdir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    mConfigFile.setFileName(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() +
                            CONFIG_FILE_NAME.toString());

    qDebug() << mConfigFile.fileName();

    if (!mConfigFile.open(QIODevice::ReadWrite | QIODevice::ExistingOnly | QIODevice::Text)) {
        setConfigReady(true);
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(mConfigFile.readAll());
    mConfigFile.close();
    if (!jsonDoc.isObject()) {
        qWarning() << "Config file is not a valid JSON object";
        setConfigReady(true);
        return;
    }

    mConfig = jsonDoc.object().toVariantHash();
    parseConfig();
}

Config::~Config() { saveToFile(); }

QVariantHash Config::getConfig() const { return mConfig; }
void Config::setConfig(const QVariantHash &data) {
    if (mConfig == data) {
        return;
    }
    mConfig = data;
    parseConfig();
    saveToFile();
    emit configChanged();
}

const QList<QDir> &Config::getModsDirs() const { return mModsDirs; }

bool Config::isReady() const { return mConfigReady; }

void Config::saveToFile() const {
    QSaveFile newConfigFile{mConfigFile.fileName()};
    if (!newConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Config could not be saved";
        return;
    }
    newConfigFile.write(QJsonDocument(QJsonObject::fromVariantHash(mConfig)).toJson());
    if (!newConfigFile.commit()) {
        qWarning() << "Config could not be saved";
        return;
    }
    qDebug() << "Config saved";
}

void Config::setConfigReady(bool ready) {
    mConfigReady = ready;
    emit configReady();
}

void Config::parseConfig() {
    mModsDirs.clear();
    using namespace Qt::StringLiterals;
    const QString clientSettingsFilename = "clientsettings.json";

    if (!mConfig[GENERAL_JSON_KEY].isValid() || mConfig[GENERAL_JSON_KEY].isNull() ||
        !mConfig[GENERAL_JSON_KEY].canConvert<QVariantHash>()) {
        qWarning() << "VSMM config is not a valid";
        setConfigReady(true);
        return;
    }

    auto vsmm = mConfig[GENERAL_JSON_KEY].toHash();
    auto configGamePath = vsmm["configGamePath"_L1].toString();
    QDir configGameDir{configGamePath};
    if (configGamePath.isEmpty() || !configGameDir.exists()) {
        qWarning() << "Config game path does not exist";
        setConfigReady(true);
        return;
    }

    if (!configGameDir.exists(clientSettingsFilename)) {
        qWarning() << "Client settings file does not exist";
        setConfigReady(true);
        return;
    }

    QFile clientSettingsFile{configGamePath + QDir::separator() + clientSettingsFilename};
    if (!clientSettingsFile.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::ExistingOnly)) {
        qWarning() << u"Failed to open client settings file"_s.arg(clientSettingsFile.fileName());
        setConfigReady(true);
        return;
    }

    QJsonDocument clientSettingsDoc = QJsonDocument::fromJson(clientSettingsFile.readAll());
    if (!clientSettingsDoc.isObject()) {
        qWarning() << "Client settings file is not a valid JSON object";
        setConfigReady(true);
        return;
    }

    auto clientSettings = clientSettingsDoc.object();
    if (const auto &[valid, reason] = checkClientSettingsVer(clientSettings); !valid) {
        qWarning() << u"Detected unsupported clientsettings: %1"_s.arg(reason);
        setConfigReady(true);
        return;
    }

    readModsPaths(clientSettings);
    emit configChanged();
    setConfigReady(true);
}

void Config::readModsPaths(const QJsonObject &clientSettings) {
    using namespace Qt::StringLiterals;
    if (!clientSettings["stringListSettings"_L1].isObject()) {
        qWarning() << "Failed to locate stringListSettings in client settings file";
        return;
    }

    auto stringListSettings = clientSettings["stringListSettings"_L1].toObject();
    if (!stringListSettings["modPaths"_L1].isArray()) {
        qWarning() << "Invalid modPaths";
        return;
    }

    for (const auto &path : stringListSettings["modPaths"_L1].toArray()) {
        if (path == "Mods" || path.isNull() || path.isUndefined() || path.toStringView().isEmpty()) {
            continue;
        }

        mModsDirs.append(path.toString());
    }
}

QPair<bool, QString> Config::checkClientSettingsVer(const QJsonObject &clientSettings) const {
    using namespace Qt::StringLiterals;
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