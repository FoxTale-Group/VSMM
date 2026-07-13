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
#include <QStandardPaths>

Q_STATIC_LOGGING_CATEGORY(cConfig, "config");

namespace vsmm {
Config::Config() {
    QDir().mkdir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    mConfigFile.setFileName(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() +
                            CONFIG_FILE_NAME.toString());

    if (!mConfigFile.open(QIODevice::ReadWrite | QIODevice::ExistingOnly | QIODevice::Text)) {
        qCWarning(cConfig, "Could not open config file");
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(mConfigFile.readAll(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        qCFatal(cConfig) << jsonError.errorString();
        return;
    }

    mConfigFile.close();
    if (!jsonDoc.isObject()) {
        qCWarning(cConfig, "Config file is not a valid JSON object");
        return;
    }

    mConfig = jsonDoc.object().toVariantHash();
}

Config::~Config() { saveToFile(); }

QString Config::getPath(QLatin1StringView key) const { return mConfig[PATHS_JSON_KEY].toHash()[key].toString(); }
QStringList Config::getFavorites() const { return mConfig[FAVORITES_JSON_KEY].toStringList(); }
void Config::setFavorites(QStringList favorites) {
    mConfig[FAVORITES_JSON_KEY] = QVariant::fromValue(std::move(favorites));
    saveToFile();
}

QVariantHash Config::getGeneral() const { return mConfig[GENERAL_JSON_KEY].toHash(); }
QVariantHash Config::getPaths() const { return mConfig[PATHS_JSON_KEY].toHash(); }
QVariantHash Config::getAppearance() const { return mConfig[APPEARANCE_JSON_KEY].toHash(); }

void Config::setGeneral(const QVariantHash &data) {
    if (mConfig[GENERAL_JSON_KEY].toHash() == data) {
        return;
    }

    mConfig[GENERAL_JSON_KEY] = QVariant::fromValue(data);
    saveToFile();
    emit generalChanged();
    qCDebug(cConfig) << "emitted generalChanged";
}

void Config::setPaths(const QVariantHash &data) {
    using namespace Qt::StringLiterals;
    constexpr QLatin1StringView CONFIG_DIR_KEY_NAME{"gameConfig"};

    if (mConfig[PATHS_JSON_KEY].toHash() == data) {
        return;
    }

    const QDir oldConfigDir = getPath(CONFIG_DIR_KEY_NAME);

    mConfig[PATHS_JSON_KEY] = QVariant::fromValue(data);
    saveToFile();
    emit pathsChanged();
    qCDebug(cConfig) << "emitted pathsChanged";

    // Check if config dir path changed
    if (oldConfigDir != mConfig[PATHS_JSON_KEY].toHash()[CONFIG_DIR_KEY_NAME].toString()) {
        emit gameConfigPathChanged();
        qCDebug(cConfig) << "emitted gameConfigPathChanged";
    }
}

void Config::setAppearance(const QVariantHash &data) {
    if (mConfig[APPEARANCE_JSON_KEY].toHash() == data) {
        return;
    }

    mConfig[APPEARANCE_JSON_KEY] = QVariant::fromValue(data);
    saveToFile();
    emit appearanceChanged();
    qCDebug(cConfig) << "emitted appearanceChanged";
}

void Config::saveToFile() const {
    QSaveFile newConfigFile{mConfigFile.fileName()};
    if (!newConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(cConfig, "Failed to open config file for writing");
        return;
    }
    newConfigFile.write(QJsonDocument(QJsonObject::fromVariantHash(mConfig)).toJson());
    if (!newConfigFile.commit()) {
        qCWarning(cConfig, "Failed to write config file");
        return;
    }
    qCInfo(cConfig, "Config saved");
}

void Config::validate() {
    using namespace Qt::StringLiterals;

    if (!mConfig[GENERAL_JSON_KEY].isValid() || mConfig[GENERAL_JSON_KEY].isNull() ||
        !mConfig[GENERAL_JSON_KEY].canConvert<QVariantHash>()) {
        qCCritical(cConfig, "VSMM config is not a valid");
        return;
    }

    auto general = mConfig[PATHS_JSON_KEY].toHash();
    auto configGamePath = general["gameConfig"_L1].toString();
    QDir configGameDir{configGamePath};
    if (configGamePath.isEmpty() || !configGameDir.exists()) {
        qCCritical(cConfig, "Config game path does not exist");
        return;
    }

    emit generalChanged();
    emit appearanceChanged();
    emit pathsChanged();
    emit gameConfigPathChanged();
    qCDebug(cConfig) << "emitted QML signals & gameConfigPathChanged";
}

} // namespace vsmm