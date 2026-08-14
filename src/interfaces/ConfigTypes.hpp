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

#include <QMetaEnum>
#include <QObject>
#include <QString>
#include <QVariantHash>

namespace vsmm {

namespace appearance {
Q_NAMESPACE_EXPORT(INTERFACES_EXPORT)
enum class Theme { Dark, Light };
Q_ENUM_NS(Theme)
} // namespace appearance

inline QString themeToString(appearance::Theme theme) {
    const QMetaEnum meta = QMetaEnum::fromType<appearance::Theme>();
    return QString::fromLatin1(meta.valueToKey(static_cast<int>(theme))).toLower();
}

inline appearance::Theme themeFromString(const QString &name) {
    const QMetaEnum meta = QMetaEnum::fromType<appearance::Theme>();
    for (int i = 0; i < meta.keyCount(); ++i) {
        if (!name.compare(QLatin1StringView{meta.key(i)}, Qt::CaseInsensitive)) {
            return static_cast<appearance::Theme>(meta.value(i));
        }
    }
    return appearance::Theme::Dark;
}

struct INTERFACES_EXPORT GeneralSettings {
    Q_GADGET
    Q_PROPERTY(bool deleteOldModVersion MEMBER deleteOldModVersion)
    Q_PROPERTY(bool includeModPrerelease MEMBER includeModPrerelease)

  public:
    static constexpr QLatin1StringView KEY_NAME{"general"};

    static constexpr bool DEFAULT_VALUE_DELETE_OLD_MOD_VERSION = true;
    static constexpr QLatin1StringView DELETE_OLD_MOD_VERSION_KEY{"deleteOldModVersion"};

    static constexpr bool DEFAULT_VALUE_INCLUDE_MOD_PRERELEASE = false;
    static constexpr QLatin1StringView INCLUDE_MOD_PRERELEASE_KEY{"includeModPrerelease"};

    bool deleteOldModVersion{DEFAULT_VALUE_DELETE_OLD_MOD_VERSION};
    bool includeModPrerelease{DEFAULT_VALUE_INCLUDE_MOD_PRERELEASE};

    bool operator==(const GeneralSettings &) const = default;

    [[nodiscard]] static GeneralSettings fromHash(const QVariantHash &hash) {
        GeneralSettings settings;
        settings.deleteOldModVersion =
            hash.value(DELETE_OLD_MOD_VERSION_KEY, DEFAULT_VALUE_DELETE_OLD_MOD_VERSION).toBool();
        settings.includeModPrerelease =
            hash.value(INCLUDE_MOD_PRERELEASE_KEY, DEFAULT_VALUE_INCLUDE_MOD_PRERELEASE).toBool();
        return settings;
    }
    [[nodiscard]] QVariantHash toHash() const {
        QVariantHash hash;
        hash.insert(DELETE_OLD_MOD_VERSION_KEY, deleteOldModVersion);
        hash.insert(INCLUDE_MOD_PRERELEASE_KEY, includeModPrerelease);
        return hash;
    }
};

struct INTERFACES_EXPORT PathSettings {
    Q_GADGET
    Q_PROPERTY(QString gameConfig MEMBER gameConfig)
    Q_PROPERTY(QString gameExe MEMBER gameExe)

  public:
    static constexpr QLatin1StringView KEY_NAME{"paths"};

    static constexpr QLatin1StringView GAME_CONFIG_KEY{"gameConfig"};
    static constexpr QLatin1StringView GAME_EXE_KEY{"gameExe"};

    QString gameConfig{};
    QString gameExe{};

    bool operator==(const PathSettings &) const = default;

    [[nodiscard]] static PathSettings fromHash(const QVariantHash &hash) {
        PathSettings settings;
        settings.gameConfig = hash.value(GAME_CONFIG_KEY).toString();
        settings.gameExe = hash.value(GAME_EXE_KEY).toString();
        return settings;
    }
    [[nodiscard]] QVariantHash toHash() const {
        QVariantHash hash;
        hash.insert(GAME_CONFIG_KEY, gameConfig);
        hash.insert(GAME_EXE_KEY, gameExe);
        return hash;
    }
};

struct INTERFACES_EXPORT AppearanceSettings {
    Q_GADGET
    Q_PROPERTY(vsmm::appearance::Theme theme MEMBER theme)
    Q_PROPERTY(int accentIndex MEMBER accentIndex)

  public:
    static constexpr QLatin1StringView KEY_NAME{"appearance"};

    static constexpr auto DEFAULT_VALUE_THEME = appearance::Theme::Dark;
    static constexpr QLatin1StringView THEME_KEY{"theme"};

    static constexpr auto DEFAULT_VALUE_ACCENT_IDX = 0;
    static constexpr QLatin1StringView ACCENT_IDX_KEY{"accentIndex"};

    appearance::Theme theme{DEFAULT_VALUE_THEME};
    int accentIndex{DEFAULT_VALUE_ACCENT_IDX};

    bool operator==(const AppearanceSettings &) const = default;

    [[nodiscard]] static AppearanceSettings fromHash(const QVariantHash &hash) {
        AppearanceSettings settings;
        settings.theme = themeFromString(hash.value(THEME_KEY).toString());
        settings.accentIndex = hash.value(ACCENT_IDX_KEY, DEFAULT_VALUE_ACCENT_IDX).toInt();
        return settings;
    }
    [[nodiscard]] QVariantHash toHash() const {
        QVariantHash hash;
        hash.insert(THEME_KEY, themeToString(theme));
        hash.insert(ACCENT_IDX_KEY, accentIndex);
        return hash;
    }
};

} // namespace vsmm