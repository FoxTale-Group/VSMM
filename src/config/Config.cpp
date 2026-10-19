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
#include <QLoggingCategory>
#include <QMetaEnum>

Q_STATIC_LOGGING_CATEGORY(cConfig, "config");

namespace vsmm {
Config::Config() {
    qCDebug(cConfig, "Using %s for settings", qUtf8Printable(mSettings.fileName()));
    mGeneral = GeneralSettings::load(mSettings);
    mPaths = PathSettings::load(mSettings);
    mAppearance = AppearanceSettings::load(mSettings);

    mFavorites = mSettings.value(FAVORITES_KEY_NAME).toStringList();

    qCInfo(cConfig, "Config loaded");
}

Config::~Config() { save(); }

void Config::setFavorites(QStringList favorites) {
    mFavorites = std::move(favorites);
    save();
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

void Config::save() {
    mGeneral.save(mSettings);
    mPaths.save(mSettings);
    mAppearance.save(mSettings);
    if (mFavorites.isEmpty()) {
        mSettings.remove(FAVORITES_KEY_NAME);
    } else {
        mSettings.setValue(FAVORITES_KEY_NAME, mFavorites);
    }

    mSettings.sync();

    if (const auto status = mSettings.status(); status != QSettings::NoError) {
        const QMetaEnum statusEnum = QMetaEnum::fromType<QSettings::Status>();
        qCWarning(cConfig, "Failed to write config file: %s", statusEnum.valueToKey(status));
        return;
    }

    qCInfo(cConfig, "Config saved to file");
}

void Config::validate() {
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