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

#include <InterfacesExport.hpp>
#include <QDir>
#include <QList>
#include <QObject>
#include <QString>

#include <semver.hpp>

#include <optional>

namespace vsmm {
class INTERFACES_EXPORT IGameMngr : public QObject {
    Q_OBJECT
    // empty if the version is unknown
    Q_PROPERTY(QString gameVersion READ getGameVersionString NOTIFY gameVersionChanged)

  public:
    ~IGameMngr() override;

    [[nodiscard]] virtual const QList<QDir> &getModsDirs() const = 0;
    [[nodiscard]] virtual const std::optional<semver::version<>> &getGameVersion() const = 0;
    // non-virtual, every implementation formats the version the same way
    [[nodiscard]] QString getGameVersionString() const {
        const std::optional<semver::version<>> &version = getGameVersion();
        return version ? QString::fromStdString(version->to_string()) : QString{};
    }
    Q_INVOKABLE virtual void launchGame() const = 0;

  signals:
    void modsDirsChanged();    // used by modstore
    void gameVersionChanged(); // used by qml
};
} // namespace vsmm
