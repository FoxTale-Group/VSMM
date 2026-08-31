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

#include <IGameMngr.hpp>

#include <utility>

namespace vsmm {
class GameMngrMock final : public IGameMngr {
  public:
    ~GameMngrMock() override = default;

    [[nodiscard]] const QList<QDir> &getModsDirs() const override { return mModsDirs; }
    [[nodiscard]] const std::optional<semver::version<>> &getGameVersion() const override { return mGameVersion; }
    // records instead of spawning a process
    void launchGame() const override { ++mLaunchCount; }

    void setModsDirs(QList<QDir> modsDirs) {
        mModsDirs = std::move(modsDirs);
        emit modsDirsChanged();
    }

    void setGameVersion(std::optional<semver::version<>> version) {
        mGameVersion = std::move(version);
        emit gameVersionChanged();
    }

    [[nodiscard]] int launchCount() const { return mLaunchCount; }

  private:
    QList<QDir> mModsDirs;
    std::optional<semver::version<>> mGameVersion;
    mutable int mLaunchCount{0};
};
} // namespace vsmm
