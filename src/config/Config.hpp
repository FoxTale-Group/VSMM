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

#include <ConfigExport.hpp>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QSaveFile>
#include <qqmlintegration.h>

namespace vsmm {
class CONFIG_EXPORT Config : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Config)
    QML_SINGLETON
    Q_PROPERTY(QVariantHash general READ getGeneral WRITE setGeneral NOTIFY generalChanged)
    Q_PROPERTY(QVariantHash paths READ getPaths WRITE setPaths NOTIFY pathsChanged)
    Q_PROPERTY(QVariantHash appearance READ getAppearance WRITE setAppearance NOTIFY appearanceChanged)

    static constexpr QLatin1StringView CONFIG_FILE_NAME{"config.json"};
    static constexpr QLatin1StringView GENERAL_JSON_KEY{"general"};
    static constexpr QLatin1StringView APPEARANCE_JSON_KEY{"appearance"};
    static constexpr QLatin1StringView PATHS_JSON_KEY{"paths"};
    static constexpr QLatin1StringView FAVORITES_JSON_KEY{"favorites"};

  public:
    static constexpr QLatin1StringView DELETE_OLD_VERSION_JSON_KEY{"deleteOldModVersion"};
    static constexpr QLatin1StringView INCLUDE_MOD_PRERELEASE_JSON_KEY{"includeModPrerelease"};

    Config();
    ~Config() override;
    void validate();

    [[nodiscard]] QString getPath(QLatin1StringView key) const;
    [[nodiscard]] QStringList getFavorites() const;

    template <typename T> [[nodiscard]] T getGeneral(QLatin1StringView key) const {
        return mConfig[GENERAL_JSON_KEY].toHash()[key].value<T>();
    }
    template <typename T> [[nodiscard]] T getAppearance(QLatin1StringView key) const {
        return mConfig[APPEARANCE_JSON_KEY].toHash()[key].value<T>();
    }

    void setFavorites(QStringList favorites);

  signals:
    void generalChanged();    // NOTIFY config — QML bindings only
    void pathsChanged();      // NOTIFY config — QML bindings only
    void appearanceChanged(); // NOTIFY config — QML bindings only

    void gameConfigPathChanged(); // used by gamemngr

  private:
    [[nodiscard]] QVariantHash getGeneral() const;
    [[nodiscard]] QVariantHash getPaths() const;
    [[nodiscard]] QVariantHash getAppearance() const;

    void setGeneral(const QVariantHash &data);
    void setPaths(const QVariantHash &data);
    void setAppearance(const QVariantHash &data);

    void saveToFile() const;
    QVariantHash mConfig;
    QFile mConfigFile;
    bool mConfigReady{false};
};
} // namespace vsmm