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
#include <GameMngr.hpp>
#include <GameMngrTestUtils.hpp>
#include <ModStore.hpp>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace vsmm::test {

// real Config, GameMngr and ModStore connected the way App does it, over a temp game dir and a fake game exe
class RealComponents {
  public:
    struct Options {
        // old zips are kept by default, so nothing reaches the real trash
        QJsonObject mGeneral{{GeneralSettings::DELETE_OLD_MOD_VERSION_KEY, false}};
        QStringList mFavorites;
        QString mGameVersionOutput{u"1.22.5"_s};
        // dir names under the temp dir, listed in clientsettings after mods
        QStringList mExtraModsDirs;
    };

    [[nodiscard]] static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    [[nodiscard]] static QString configFilePath() {
        return configDir() + QDir::separator() + IConfig::CONFIG_FILE_NAME;
    }

    RealComponents() = default;
    RealComponents(const RealComponents &) = delete;
    RealComponents &operator=(const RealComponents &) = delete;
    ~RealComponents() {
        stop();
        QFile::remove(configFilePath());
    }

    [[nodiscard]] bool start() { return start(Options{}); }

    // false when a step failed, the failure is already reported against the running test
    [[nodiscard]] bool start(const Options &options) {
        if (!check(mTempDir.isValid(), "temp dir is valid")) {
            return false;
        }
        QStringList modPaths{modsDir().absolutePath()};
        for (const auto &name : options.mExtraModsDirs) {
            modPaths.append(dir(name).absolutePath());
        }
        for (const auto &name : QStringList{u"game"_s, u"mods"_s, u"staging"_s} + options.mExtraModsDirs) {
            if (!check(root().mkpath(name), "mkpath")) {
                return false;
            }
        }
        if (!check(writeClientSettings(gameDir(), clientSettings(QJsonArray::fromStringList(modPaths))),
                   "clientsettings.json written")) {
            return false;
        }
        const QString exe = writeFakeGameExe(root(), options.mGameVersionOutput);
        if (!check(!exe.isEmpty(), "fake game exe written") ||
            !check(writeConfig(options, exe), "config.json written")) {
            return false;
        }
        return launch();
    }

    // tears everything down and brings it back from the same files, like the app restarting
    [[nodiscard]] bool restart() {
        stop();
        return launch();
    }

    [[nodiscard]] Config &config() const { return *mConfig; }
    [[nodiscard]] GameMngr &gameMngr() const { return *mGameMngr; }
    [[nodiscard]] ModStore &store() const { return *mStore; }

    [[nodiscard]] QDir root() const { return QDir{mTempDir.path()}; }
    [[nodiscard]] QDir dir(const QString &name) const { return QDir{mTempDir.filePath(name)}; }
    [[nodiscard]] QDir gameDir() const { return dir(u"game"_s); }
    [[nodiscard]] QDir modsDir() const { return dir(u"mods"_s); }
    // stands in for wherever a zip is picked up from, a download or a second mods folder
    [[nodiscard]] QDir stagingDir() const { return dir(u"staging"_s); }

  private:
    [[nodiscard]] static bool check(bool ok, const char *step) {
        return QTest::qVerify(ok, step, "RealComponents setup", __FILE__, __LINE__);
    }

    // config.json as the app would find it on disk
    [[nodiscard]] bool writeConfig(const Options &options, const QString &gameExe) const {
        if (!QDir().mkpath(configDir())) {
            return false;
        }
        const QJsonObject paths{{PathSettings::GAME_CONFIG_KEY, gameDir().absolutePath()},
                                {PathSettings::GAME_EXE_KEY, gameExe}};
        const QJsonObject config{{GeneralSettings::KEY_NAME, options.mGeneral},
                                 {PathSettings::KEY_NAME, paths},
                                 {IConfig::FAVORITES_KEY_NAME, QJsonArray::fromStringList(options.mFavorites)}};
        return writeFile(configFilePath(), QJsonDocument{config}.toJson());
    }

    [[nodiscard]] bool launch() {
        mConfig = std::make_unique<Config>();
        mGameMngr = std::make_unique<GameMngr>();
        mGameMngr->setConfig(mConfig.get());
        mStore = std::make_unique<ModStore>();
        mStore->setConfig(mConfig.get());
        mStore->setGameMngr(mGameMngr.get());

        // the first scan is announced only once the version read settled
        const QSignalSpy scan{mGameMngr.get(), &IGameMngr::modsDirsChanged};
        mConfig->validate();
        return check(QTest::qWaitFor([&scan] { return scan.count() == 1; }), "first scan announced");
    }

    // Config saves on destruction, so it goes last
    void stop() {
        mStore.reset();
        mGameMngr.reset();
        mConfig.reset();
    }

    QTemporaryDir mTempDir;
    std::unique_ptr<Config> mConfig;
    std::unique_ptr<GameMngr> mGameMngr;
    std::unique_ptr<ModStore> mStore;
};
} // namespace vsmm::test
