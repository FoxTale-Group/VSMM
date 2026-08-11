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

#include "ConfigTypes.hpp"
#include <InterfacesExport.hpp>
#include <QObject>
#include <QStringList>
#include <QVariant>

namespace vsmm {
class INTERFACES_EXPORT IConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(vsmm::GeneralSettings general READ general WRITE setGeneral NOTIFY generalChanged)
    Q_PROPERTY(vsmm::PathSettings paths READ paths WRITE setPaths NOTIFY pathsChanged)
    Q_PROPERTY(vsmm::AppearanceSettings appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged)

  public:
    static constexpr QLatin1StringView CONFIG_FILE_NAME{"config.json"};
    static constexpr QLatin1StringView FAVORITES_KEY_NAME{"favorites"};

    ~IConfig() override = default;

    [[nodiscard]] virtual GeneralSettings general() const { return mGeneral; }
    [[nodiscard]] virtual PathSettings paths() const { return mPaths; }
    [[nodiscard]] virtual AppearanceSettings appearance() const { return mAppearance; }

    virtual void setGeneral(const GeneralSettings &data) = 0;
    virtual void setPaths(const PathSettings &data) = 0;
    virtual void setAppearance(const AppearanceSettings &data) = 0;

    virtual void validate() = 0;

    [[nodiscard]] const QStringList &getFavorites() const { return mFavorites; }
    virtual void setFavorites(QStringList favorites) { mFavorites = std::move(favorites); }

  signals:
    void generalChanged();    // NOTIFY general - QML
    void pathsChanged();      // NOTIFY paths - QML
    void appearanceChanged(); // NOTIFY appearance - QML

    void gameConfigPathChanged(); // used by gamemngr
    void gameExePathChanged();    // used by gamemngr

  protected:
    GeneralSettings mGeneral;
    PathSettings mPaths;
    AppearanceSettings mAppearance;
    QStringList mFavorites;
};
} // namespace vsmm
