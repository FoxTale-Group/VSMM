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
#include <Config.hpp>
#include <GameMngrExport.hpp>
#include <QObject>
#include <QProcess>
#include <qqmlintegration.h>

#include <semver.hpp>

namespace vsmm {
class GAMEMNGR_EXPORT GameMngr : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(GameMngr)
    QML_SINGLETON

    static constexpr QLatin1StringView CONFIG_GAMEDIR_JSON_KEY{"gameConfig"};
    static constexpr QLatin1StringView CONFIG_GAMEEXE_JSON_KEY{"gameExe"};

    const QStringList CLIENT_SETTINGS_VER_SUPPORT = {QStringLiteral("1.16")};

  public:
    GameMngr() = default;
    void setConfig(Config *config);
    [[nodiscard]] const QList<QDir> &getModsDirs() const;
    [[nodiscard]] const semver::version<> &getGameVersion() const;
    Q_INVOKABLE void launchGame();

  signals:
    void modsDirsChanged(); // used by modstore

  private:
    void readGameVersion();
    void readModsPaths(const QJsonObject &clientSettings);
    [[nodiscard]] QPair<bool, QString> checkClientSettingsVer(const QJsonObject &clientSettings) const;

    Config *mConfig{nullptr};
    QList<QDir> mModsDirs;
    semver::version<> mGameVersion;
    QProcess mGameProcess{this};

  private slots:
    void parseClientCfg();
};
} // namespace vsmm