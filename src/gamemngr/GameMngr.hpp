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
#include <QTimer>
#include <qqmlintegration.h>

#include <semver.hpp>

#include <optional>

namespace vsmm {
class GAMEMNGR_EXPORT GameMngr : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(GameMngr)
    QML_SINGLETON
    // empty if the version is unknown
    Q_PROPERTY(QString gameVersion READ getGameVersionString NOTIFY gameVersionChanged)

    static constexpr QLatin1StringView CONFIG_GAMEDIR_JSON_KEY{"gameConfig"};
    static constexpr QLatin1StringView CONFIG_GAMEEXE_JSON_KEY{"gameExe"};

    const QStringList CLIENT_SETTINGS_VER_SUPPORT = {QStringLiteral("1.16")};

  public:
    GameMngr();
    void setConfig(Config *config);
    [[nodiscard]] const QList<QDir> &getModsDirs() const;
    [[nodiscard]] const std::optional<semver::version<>> &getGameVersion() const;
    [[nodiscard]] QString getGameVersionString() const;
    Q_INVOKABLE void launchGame() const;

  signals:
    void modsDirsChanged();    // used by modstore
    void gameVersionChanged(); // used by qml

  private:
    void initGameVersion();
    bool setupProcess(QProcess &process, const QStringList &arguments) const;
    bool beginVersionRead();
    void killVersionProcess();
    void finishVersionRead(std::optional<semver::version<>> version = {});
    void notifyModsDirsChanged();
    void readClientCfg();
    void readModsPaths(const QJsonObject &clientSettings);
    [[nodiscard]] QPair<bool, QString> checkClientSettingsVer(const QJsonObject &clientSettings) const;

    Config *mConfig{nullptr};
    QList<QDir> mModsDirs;
    std::optional<semver::version<>> mGameVersion;
    bool mModsDirsParsed{false};
    bool mModsDirsChangedPending{false};

    // version checking
    QTimer mVersionTimeout{this};
    QProcess mVersionProcess{this};

  private slots:
    void parseClientCfg();
    void refreshGameVersion();
    void onVersionProcessFinished(int errCode, QProcess::ExitStatus exitStatus);
    void onVersionProcessFailed(QProcess::ProcessError error);
    void onVersionReadTimeout();
};
} // namespace vsmm