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
    Q_PROPERTY(QUrl gameDir READ getGameDirQml WRITE setGameDirQml NOTIFY gameDirChanged)
    Q_PROPERTY(QUrl modsDir READ getModsDirQml WRITE setModsDirQml NOTIFY modsDirChanged)
    Q_PROPERTY(bool deleteOldVersion READ getDeleteOldModVersion WRITE setDeleteOldModVersion NOTIFY
                   deletedOldModVersionChanged)

  private:
    static constexpr QAnyStringView CONFIG_FILE_NAME = "config.json";

  public:
    Config();
    ~Config() override;
    [[nodiscard]] const QDir &getGameDir() const;
    [[nodiscard]] const QDir &getModsDir() const;
    [[nodiscard]] bool getDeleteOldModVersion() const;
    [[nodiscard]] QUrl getGameDirQml() const;
    [[nodiscard]] QUrl getModsDirQml() const;

    void setGameDirQml(const QUrl &dir);
    void setModsDirQml(const QUrl &dir);
    void setDeleteOldModVersion(bool deleteOldModVersion);

  signals:
    void gameDirChanged();
    void modsDirChanged();
    void deletedOldModVersionChanged();

  private:
    void parseConfig();
    QJsonObject mConfig;
    QFile mConfigFile;
    QDir mGameDir;
    QDir mModsDir;
    bool mDeleteOldModVersion{true};

    static QJsonObject createDefaultConfig();
};
} // namespace vsmodchecker
