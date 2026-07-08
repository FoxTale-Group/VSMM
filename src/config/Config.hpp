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
    Q_PROPERTY(QVariantHash config READ getConfig WRITE setConfig NOTIFY configChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY configReady)

    static constexpr QLatin1StringView CONFIG_FILE_NAME{"config.json"};
    static constexpr QLatin1StringView GENERAL_JSON_KEY{"vsmm"};
    const QStringList CLIENT_SETTINGS_VER_SUPPORT = {QStringLiteral("1.16")};

  public:
    static constexpr QLatin1StringView DELETE_OLD_VERSION_JSON_KEY{"deleteOldModVersion"};

    Config();
    ~Config() override;
    [[nodiscard]] QVariantHash getConfig() const;
    void setConfig(const QVariantHash &data);

    [[nodiscard]] const QList<QDir> &getModsDirs() const;
    [[nodiscard]] bool isReady() const;

    template <typename T> [[nodiscard]] T getGeneral(QLatin1StringView key) const {
        return mConfig[GENERAL_JSON_KEY].toHash()[key].value<T>();
    }

  signals:
    void configChanged(); // NOTIFY config — QML bindings only
    void configReady();   // NOTIFY ready — QML bindings only

  private:
    void saveToFile() const;
    void setConfigReady(bool ready);
    void parseConfig();
    void readModsPaths(const QJsonObject &clientSettings);
    [[nodiscard]] QPair<bool, QString> checkClientSettingsVer(const QJsonObject &clientSettings) const;
    QVariantHash mConfig;
    QFile mConfigFile;
    QList<QDir> mModsDirs;
    bool mConfigReady{false};
};
} // namespace vsmm
