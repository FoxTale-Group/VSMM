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

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <qqmlintegration.h>

namespace vsmodchecker {
class Config final : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Config)
    QML_SINGLETON
    Q_PROPERTY(QVariantHash config READ getConfig WRITE setConfig NOTIFY configChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY configReady)

    static constexpr QAnyStringView CONFIG_FILE_NAME = "config.json";
    const QStringList CLIENT_SETTINGS_VER_SUPPORT = {QStringLiteral("1.16")};

  public:
    Config();
    ~Config() override;
    [[nodiscard]] QVariantHash getConfig() const;
    void setConfig(const QVariantHash &data);

    [[nodiscard]] const QList<QDir> &getModsDirs() const;
    [[nodiscard]] bool isReady() const;

  signals:
    void configChanged();
    void configReady();

  private:
    void setConfigReady(bool ready);
    void parseConfig();
    void readModsPaths(const QJsonObject &clientSettings);
    [[nodiscard]] QPair<bool, QString> checkClientSettingsVer(const QJsonObject &clientSettings) const;
    QVariantHash mConfig;
    QFile mConfigFile;
    QList<QDir> mModsDirs;
    bool mConfigReady{false};
};
} // namespace vsmodchecker
