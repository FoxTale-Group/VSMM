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

#include "GameMngr.hpp"
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(cGameMngr, "gamemngr");

using namespace Qt::StringLiterals;

namespace {
constexpr int VERSION_READ_TIMEOUT_MS = 3000;
constexpr int VERSION_KILL_TIMEOUT_MS = 1000;
} // namespace

namespace vsmm {
GameMngr::GameMngr() {
    mVersionTimeout.setSingleShot(true);
    connect(&mVersionTimeout, &QTimer::timeout, this, &GameMngr::onVersionReadTimeout);
    connect(&mVersionProcess, &QProcess::finished, this, &GameMngr::onVersionProcessFinished);
    connect(&mVersionProcess, &QProcess::errorOccurred, this, &GameMngr::onVersionProcessFailed);
}

GameMngr::~GameMngr() {
    // disconnect, so ~QProcess won't emit any signals for gamemngr anymore
    if (!mVersionProcess.disconnect(this)) {
        qCWarning(cGameMngr) << "Failed to disconnect QProcess signals";
        // if failed, just block all signals
        mVersionProcess.blockSignals(true);
    }
    killVersionProcess();
}

void GameMngr::killVersionProcess() {
    if (mVersionProcess.state() == QProcess::NotRunning) {
        return;
    }

    const QSignalBlocker blocker{mVersionProcess};
    mVersionProcess.kill();
    mVersionProcess.waitForFinished(VERSION_KILL_TIMEOUT_MS);
}

void GameMngr::setConfig(Config *config) {
    mConfig = config;
    connect(mConfig, &Config::gameConfigPathChanged, this, &GameMngr::parseClientCfg);
    connect(mConfig, &Config::gameExePathChanged, this, &GameMngr::refreshGameVersion);
}

const QList<QDir> &GameMngr::getModsDirs() const { return mModsDirs; }

const semver::version<> &GameMngr::getGameVersion() const { return mGameVersion; }

bool GameMngr::isGameVersionKnown() const { return mGameVersionKnown; }

QString GameMngr::getGameVersionString() const {
    return mGameVersionKnown ? QString::fromStdString(mGameVersion.to_string()) : QString{};
}

void GameMngr::setupProcess(QProcess &process, const QStringList &arguments) const {
    const QFileInfo gameExe{mConfig->getPath(CONFIG_GAMEEXE_JSON_KEY)};

    process.setProgram(gameExe.absoluteFilePath());
    process.setWorkingDirectory(gameExe.absolutePath());
    process.setArguments(arguments);
}

void GameMngr::launchGame() const {
    if (!mConfig) {
        qCFatal(cGameMngr, "Config not set");
    }

    if (mConfig->getPath(CONFIG_GAMEEXE_JSON_KEY).isEmpty()) {
        qCCritical(cGameMngr) << u"config paths.%1 value is empty"_s.arg(CONFIG_GAMEEXE_JSON_KEY);
        return;
    }

    QProcess gameProcess;
    setupProcess(gameProcess, {});

    qint64 pid{-1};
    if (!gameProcess.startDetached(&pid)) {
        qCCritical(cGameMngr, "Failed to start game exe");
        return;
    }

    qCDebug(cGameMngr) << u"Process started as %1"_s.arg(pid);
}

void GameMngr::parseClientCfg() {
    mModsDirs.clear();
    readClientCfg();
    notifyModsDirsChanged();
}

void GameMngr::readClientCfg() {
    const QString clientSettingsFilename = "clientsettings.json";

    if (!mConfig) {
        qCFatal(cGameMngr, "Config not set");
    }

    QDir configGameDir = mConfig->getPath(CONFIG_GAMEDIR_JSON_KEY);
    if (!configGameDir.exists(clientSettingsFilename)) {
        qCCritical(cGameMngr, "Client settings file does not exist");
        return;
    }

    QFile clientSettingsFile{configGameDir.absolutePath() + QDir::separator() + clientSettingsFilename};
    if (!clientSettingsFile.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::ExistingOnly)) {
        qCCritical(cGameMngr) << u"Failed to open client settings file"_s.arg(clientSettingsFile.fileName());
        return;
    }

    QJsonDocument clientSettingsDoc = QJsonDocument::fromJson(clientSettingsFile.readAll());
    if (!clientSettingsDoc.isObject()) {
        qCCritical(cGameMngr, "Client settings file is not a valid JSON object");
        return;
    }

    auto clientSettings = clientSettingsDoc.object();
    if (const auto &[valid, reason] = checkClientSettingsVer(clientSettings); !valid) {
        qCCritical(cGameMngr) << u"Detected unsupported clientsettings: %1"_s.arg(reason);
        return;
    }

    readModsPaths(clientSettings);
}

void GameMngr::notifyModsDirsChanged() {
    mModsDirsParsed = true;

    if (mVersionProcess.state() != QProcess::NotRunning) {
        // dont reload the mods if check version of game is still running
        mModsDirsChangedPending = true;
        return;
    }

    mModsDirsChangedPending = false;
    emit modsDirsChanged();
}

void GameMngr::finishVersionRead(const semver::version<> &version, bool known) {
    const bool versionChanged = mGameVersionKnown != known || mGameVersion != version;
    // check if version changed and emit signal for QML
    if (versionChanged) {
        mGameVersion = version;
        mGameVersionKnown = known;
        emit gameVersionChanged();
    }

    // reload mods
    if (mModsDirsChangedPending || (versionChanged && mModsDirsParsed)) {
        mModsDirsChangedPending = false;
        emit modsDirsChanged();
    }
}

bool GameMngr::beginVersionRead() {
    if (!mConfig) {
        qCFatal(cGameMngr, "Config not set");
    }

    // version checking already running
    if (mVersionProcess.state() != QProcess::NotRunning) {
        qCDebug(cGameMngr, "Game version read already in progress");
        return false;
    }

    if (mConfig->getPath(CONFIG_GAMEEXE_JSON_KEY).isEmpty()) {
        qCCritical(cGameMngr) << u"config paths.%1 value is empty"_s.arg(CONFIG_GAMEEXE_JSON_KEY);
        finishVersionRead({}, false);
        return false;
    }

    setupProcess(mVersionProcess, {u"--version"_s});
    mVersionProcess.start();
    return true;
}

bool GameMngr::readGameVersion() {
    if (!beginVersionRead()) {
        return false;
    }

    if (!mVersionProcess.waitForFinished(VERSION_READ_TIMEOUT_MS)) {
        if (mVersionProcess.error() == QProcess::Timedout) {
            onVersionReadTimeout();
        }
        return false;
    }

    return mGameVersionKnown;
}

void GameMngr::refreshGameVersion() {
    if (!beginVersionRead()) {
        return;
    }

    mVersionTimeout.start(VERSION_READ_TIMEOUT_MS);
}

void GameMngr::onVersionProcessFinished(int errCode [[maybe_unused]], QProcess::ExitStatus exitStatus) {
    mVersionTimeout.stop();

    if (exitStatus != QProcess::NormalExit) {
        finishVersionRead({}, false);
        return;
    }

    semver::version<> version;
    if (const auto result = semver::parse(mVersionProcess.readAllStandardOutput().trimmed().toStdString(), version);
        !result) {
        qCCritical(cGameMngr, "Failed to parse game version");
        finishVersionRead({}, false);
        return;
    }

    qCDebug(cGameMngr) << u"Game version detected: %1"_s.arg(version.to_string());
    finishVersionRead(version, true);
}

void GameMngr::onVersionProcessFailed(QProcess::ProcessError error) {
    mVersionTimeout.stop();
    qCCritical(cGameMngr)
        << u"Game exe failed while reading the version: %1 (%2)"_s.arg(mVersionProcess.errorString()).arg(error);
    finishVersionRead({}, false);
}

void GameMngr::onVersionReadTimeout() {
    if (mVersionProcess.state() == QProcess::NotRunning) {
        return;
    }

    qCCritical(cGameMngr) << u"Game exe did not exit for --version; is paths.%1 the right binary?"_s.arg(
        CONFIG_GAMEEXE_JSON_KEY);
    killVersionProcess();
    finishVersionRead({}, false);
}

void GameMngr::readModsPaths(const QJsonObject &clientSettings) {
    using namespace Qt::StringLiterals;
    if (!clientSettings["stringListSettings"_L1].isObject()) {
        qCCritical(cGameMngr, "Failed to locate stringListSettings in client settings file");
        return;
    }

    auto stringListSettings = clientSettings["stringListSettings"_L1].toObject();
    if (!stringListSettings["modPaths"_L1].isArray()) {
        qCCritical(cGameMngr, "Invalid modPaths");
        return;
    }

    for (const auto &path : stringListSettings["modPaths"_L1].toArray()) {
        if (path.toStringView() == "Mods"_L1 || path.isNull() || path.isUndefined() || path.toStringView().isEmpty()) {
            continue;
        }

        mModsDirs.append(path.toString());
    }
}

QPair<bool, QString> GameMngr::checkClientSettingsVer(const QJsonObject &clientSettings) const {
    if (!clientSettings["stringSettings"_L1].isObject()) {
        return {false, u"Invalid stringSettings"_s};
    }

    auto stringSettings = clientSettings["stringSettings"_L1].toObject();
    if (auto clSettingsVersion = stringSettings["settingsVersion"_L1].toStringView();
        !CLIENT_SETTINGS_VER_SUPPORT.contains(clSettingsVersion)) {
        return {false, u"Unsupported clientsettings version %1"_s.arg(clSettingsVersion)};
    }

    return {true, {}};
}
} // namespace vsmm