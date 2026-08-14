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

#pragma once

#include <IConfig.hpp>

#include <utility>

namespace vsmm {
class ConfigMock final : public IConfig {
  public:
    // send the same signals as Config::validate()
    void validate() override {
        emit generalChanged();
        emit appearanceChanged();
        emit pathsChanged();
        emit gameConfigPathChanged();
    }

    void setFavorites(QStringList favorites) override { mFavorites = std::move(favorites); }

    void setGamePaths(const QString &gameConfigDir, const QString &gameExe) {
        mPaths.gameConfig = gameConfigDir;
        mPaths.gameExe = gameExe;
        emit gameExePathChanged();
        emit gameConfigPathChanged();
    }

    void setGameConfigDir(const QString &gameConfigDir) {
        mPaths.gameConfig = gameConfigDir;
        emit gameConfigPathChanged();
    }

    void setGameExePath(const QString &gameExe) {
        mPaths.gameExe = gameExe;
        emit gameExePathChanged();
    }

    void setGeneral(const GeneralSettings &data) override {
        mGeneral = data;
        emit generalChanged();
    }

    void setPaths(const PathSettings &data) override {
        mPaths = data;
        emit pathsChanged();
        emit gameExePathChanged();
        emit gameConfigPathChanged();
    }

    void setAppearance(const AppearanceSettings &data) override {
        mAppearance = data;
        emit appearanceChanged();
    }
};
} // namespace vsmm
