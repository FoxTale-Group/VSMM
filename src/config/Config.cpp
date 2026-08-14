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
#include <QLoggingCategory>
#include <QSaveFile>
#include <QStandardPaths>

Q_STATIC_LOGGING_CATEGORY(cConfig, "config");

namespace vsmm {
Config::Config() {
    if (!QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))) {
        qCWarning(cConfig, "Could not create config directory");
    }
    mConfigFile.setFileName(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() +
                            CONFIG_FILE_NAME.toString());

    if (!mConfigFile.open(QIODevice::ReadWrite | QIODevice::ExistingOnly | QIODevice::Text)) {
        qCWarning(cConfig, "Could not open config file");
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(mConfigFile.readAll(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        qCFatal(cConfig, "Config file is not valid JSON: %s", qUtf8Printable(jsonError.errorString()));
        return;
    }

    mConfigFile.close();
    if (!jsonDoc.isObject()) {
        qCWarning(cConfig, "Config file is not a valid JSON object");
        return;
    }

    auto config = jsonDoc.object().toVariantHash();
    if (!config[GeneralSettings::KEY_NAME].isNull() && config[GeneralSettings::KEY_NAME].isValid()) {
        mGeneral = GeneralSettings::fromHash(config[GeneralSettings::KEY_NAME].toHash());
    }
    if (!config[PathSettings::KEY_NAME].isNull() && config[PathSettings::KEY_NAME].isValid()) {
        mPaths = PathSettings::fromHash(config[PathSettings::KEY_NAME].toHash());
    }
    if (!config[AppearanceSettings::KEY_NAME].isNull() && config[AppearanceSettings::KEY_NAME].isValid()) {
        mAppearance = AppearanceSettings::fromHash(config[AppearanceSettings::KEY_NAME].toHash());
    }
    if (!config[FAVORITES_KEY_NAME].isNull() && config[FAVORITES_KEY_NAME].isValid()) {
        mFavorites = config[FAVORITES_KEY_NAME].toStringList();
    }

    qCInfo(cConfig, "Config loaded");
}

Config::~Config() { saveToFile(); }

void Config::setFavorites(QStringList favorites) {
    mFavorites = std::move(favorites);
    saveToFile();
    qCDebug(cConfig, "Favorites set to %s", qUtf8Printable(mFavorites.join(QStringLiteral(", "))));
}

void Config::setGeneral(const GeneralSettings &data) {
    if (general() == data) {
        return;
    }
    mGeneral = data;
    emit generalChanged();
    qCInfo(cConfig, "General settings changed");
}

void Config::setPaths(const PathSettings &data) {
    const PathSettings oldPaths = paths();
    if (oldPaths == data) {
        return;
    }

    mPaths = data;
    emit pathsChanged();

    if (oldPaths.gameExe != data.gameExe) {
        emit gameExePathChanged();
        qCDebug(cConfig, "Game exe path changed");
    }

    if (oldPaths.gameConfig != data.gameConfig) {
        emit gameConfigPathChanged();
        qCDebug(cConfig, "Game config path changed");
    }

    qCInfo(cConfig, "Paths settings changed");
}

void Config::setAppearance(const AppearanceSettings &data) {
    if (appearance() == data) {
        return;
    }

    mAppearance = data;
    emit appearanceChanged();
    qCInfo(cConfig, "Appearance settings changed");
}

void Config::saveToFile() const {
    QSaveFile newConfigFile{mConfigFile.fileName()};
    if (!newConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(cConfig, "Failed to open config file for writing");
        return;
    }
    QVariantHash configHash;
    configHash[GeneralSettings::KEY_NAME] = mGeneral.toHash();
    configHash[PathSettings::KEY_NAME] = mPaths.toHash();
    configHash[AppearanceSettings::KEY_NAME] = mAppearance.toHash();
    configHash[FAVORITES_KEY_NAME] = mFavorites;
    newConfigFile.write(QJsonDocument(QJsonObject::fromVariantHash(configHash)).toJson());
    if (!newConfigFile.commit()) {
        qCWarning(cConfig, "Failed to write config file");
        return;
    }
    qCInfo(cConfig, "Config saved to file");
}

void Config::validate() {
    using namespace Qt::StringLiterals;

    const auto &configGamePath = mPaths.gameConfig;
    if (const QDir configGameDir{configGamePath}; configGamePath.isEmpty() || !configGameDir.exists()) {
        qCCritical(cConfig, "Config game path does not exist");
        return;
    }

    emit generalChanged();
    emit appearanceChanged();
    emit pathsChanged();
    emit gameConfigPathChanged();
    qCInfo(cConfig, "Config validated");
}

} // namespace vsmm