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

#include "ModLoader.hpp"
#include "constants.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>

#include <algorithm>

#include <ZipArchive.hpp>
#include <semver.hpp>
#include <utility>

Q_STATIC_LOGGING_CATEGORY(cModLoader, "modloader");

namespace {
using namespace Qt::StringLiterals;
QUrl GetModUrlApi(const QUrl &apiUrl, QAnyStringView modId) { return u"%1%2"_s.arg(apiUrl.toString(), modId); }
} // namespace

namespace vsmm {
ModLoader::ModLoader() {
    mThreadPoolExtractZips.setMaxThreadCount(std::min(4, QThread::idealThreadCount()));
    mThreadPoolProcessUpdate.setMaxThreadCount(std::min(4, QThread::idealThreadCount()));
}

bool ModLoader::initModsList() {
    static const QStringList modsExts{{"*.zip"}};

    // GCOVR_EXCL_START
    if (!mGameMngr) {
        qCFatal(cModLoader, "GameMngr is not set");
    }
    // GCOVR_EXCL_STOP

    QList<QDir> modsDirs = mGameMngr->getModsDirs();
    if (modsDirs.isEmpty()) {
        if (!mModsLoadingInProgress.loadRelaxed()) {
            emit allModsReloaded();
        }
        qCCritical(cModLoader, "No mods dirs found");
        return false;
    }

    int scanned{0};
    for (const auto &modsDir : modsDirs) {
        if (!modsDir.exists()) {
            qCWarning(cModLoader, "Mods path %s does not exist", qUtf8Printable(modsDir.path()));
            continue;
        }

        auto entries = modsDir.entryInfoList(modsExts, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
        qCDebug(cModLoader, "Scanning %s: %lld zips", qUtf8Printable(modsDir.path()),
                static_cast<long long>(entries.size()));
        scanned += static_cast<int>(entries.size());

        for (auto &entry : entries) {
            load(std::move(entry));
        }
    }

    qCInfo(cModLoader, "Scan started for %d mods across %lld dirs", scanned, static_cast<long long>(modsDirs.size()));

    if (!mModsLoadingInProgress.loadRelaxed()) {
        emit allModsReloaded();
    }
    return true;
}

void ModLoader::setHttpClient(IHttpClient *httpClient) {
    if (mHttpClient) {
        qCWarning(cModLoader, "HttpClient already set");
        return;
    }
    // GCOVR_EXCL_START
    if (!httpClient) {
        qCFatal(cModLoader, "HttpClient is null");
        return;
    }
    // GCOVR_EXCL_STOP

    mHttpClient = httpClient;
}

void ModLoader::setGameMngr(IGameMngr *gameMngr) {
    if (mGameMngr) {
        qCWarning(cModLoader, "GameMngr already set");
        return;
    }
    // GCOVR_EXCL_START
    if (!gameMngr) {
        qCFatal(cModLoader, "GameMngr is null");
        return;
    }
    // GCOVR_EXCL_STOP

    mGameMngr = gameMngr;
}

void ModLoader::setStore(IModStore *store) {
    if (mStore) {
        qCWarning(cModLoader, "ModStore already set");
        return;
    }
    // GCOVR_EXCL_START
    if (!store) {
        qCFatal(cModLoader, "ModStore is null");
        return;
    }
    // GCOVR_EXCL_STOP

    mStore = store;
    connect(mStore, &IModStore::modsReloading, this, &ModLoader::onModsReloading);
    connect(mStore, &IModStore::modUpdateRequested, this, &ModLoader::onModUpdateRequested);
    connect(this, &ModLoader::allModsReloaded, mStore, &IModStore::onModsReloaded);
}

void ModLoader::setApiUrl(QUrl apiUrl) { mApiUrl = std::move(apiUrl); }

void ModLoader::load(QFileInfo &&fileInfo) {
    incrementModsLoadingInProgress();
    load_(std::move(fileInfo), IModStore::ModLoadType::Init);
}
void ModLoader::load(const QUrl &filePath) {
    incrementModsLoadingInProgress();
    load_(QFileInfo{filePath.toLocalFile()}, IModStore::ModLoadType::GUI);
}

void ModLoader::onModUpdateRequested(QStringList modIds) {
    incrementModsLoadingInProgress(modIds.size());

    for (const auto &id : modIds) {
        const ModEntry *mod = mStore->find(id);

        // Should never happen, but check just in case
        if (!mod) {
            qCCritical(cModLoader, "Mod with id %s not found", qUtf8Printable(id));
            decrementModsLoadingInProgress(false);
            continue;
        }

        // copied, the entry is replaced once the update is installed
        ModEntry::LatestVersion latestVersion = mod->getLatestVersion();
        const QUrl url = latestVersion.mUrl;
        qCDebug(cModLoader, "Downloading %s %s from %s", qUtf8Printable(id), latestVersion.mVersion.to_string().c_str(),
                qUtf8Printable(url.toString()));
        mHttpClient->sendGet(
            url, this, DOWNLOAD_CONTENT_TYPE,
            [this, id = id, latestVersion = std::move(latestVersion)](QByteArray data) mutable {
                onModUpdateRetrieved(std::move(data), std::move(latestVersion), std::move(id));
            },
            [this, id = id](QString error) mutable {
                qCWarning(cModLoader, "Failed to retrieve update for %s: %s", qUtf8Printable(id),
                          qUtf8Printable(error));
                mStore->onModUpdateFailed(std::move(id), std::move(error));
                decrementModsLoadingInProgress();
            },
            [this, id](qint64 received, qint64 total) { mStore->onModUpdateProgress(id, received, total); });
    }

    // inform ModStore, so it would set mWorkPending = false if no other loads are pending
    if (!mModsLoadingInProgress.loadRelaxed()) {
        qCDebug(cModLoader, "Nothing left in progress");
        emit allModsReloaded();
    }
}

void ModLoader::onModUpdateRetrieved(QByteArray data, ModEntry::LatestVersion latestVersion, QString modId) {
    mThreadPoolProcessUpdate.start([this, latestVersion = std::move(latestVersion), data = std::move(data),
                                    modId = std::move(modId)] mutable {
        QFile modUpdateFile{QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() +
                            latestVersion.mFileName};
        if (!modUpdateFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate)) {
            qCWarning(cModLoader, "Failed to create file for mod update %s", qUtf8Printable(latestVersion.mFileName));
            reportUpdateFailed(std::move(modId), u"Failed to create the update file"_s);
            return;
        }
        if (modUpdateFile.write(data) != data.size()) {
            qCWarning(cModLoader, "Failed to write mod update data to %s", qUtf8Printable(modUpdateFile.fileName()));
            reportUpdateFailed(std::move(modId), u"Failed to write the update file"_s);
            return;
        }
        modUpdateFile.close();
        qCDebug(cModLoader, "Saved update for %s to %s (%lld bytes)", qUtf8Printable(modId),
                qUtf8Printable(modUpdateFile.fileName()), static_cast<long long>(data.size()));
        QFileInfo fileInfo{modUpdateFile.fileName()};
        QMetaObject::invokeMethod(
            this,
            [this, fileInfo, modId = std::move(modId)] mutable {
                load_(std::move(fileInfo), IModStore::ModLoadType::Update, std::move(modId));
            },
            Qt::QueuedConnection);
    });
}

void ModLoader::reportUpdateFailed(QString modId, QString reason) {
    // report about failure is before the decrement, so the failure reaches the store before allModsReloaded is emitted
    QMetaObject::invokeMethod(
        this,
        [this, modId = std::move(modId), reason = std::move(reason)] mutable {
            mStore->onModUpdateFailed(std::move(modId), std::move(reason));
            decrementModsLoadingInProgress();
        },
        Qt::QueuedConnection);
}

void ModLoader::incrementModsLoadingInProgress(qint64 count) { mModsLoadingInProgress.fetchAndAddRelaxed(count); }

void ModLoader::decrementModsLoadingInProgress(bool notify) {
    if (mModsLoadingInProgress.fetchAndSubRelaxed(1) == 1 && notify) {
        qCDebug(cModLoader, "Nothing left in progress");
        QMetaObject::invokeMethod(this, [this] { emit allModsReloaded(); }, Qt::QueuedConnection);
    }
}

void ModLoader::onModsReloading() { initModsList(); }

void ModLoader::load_(QFileInfo fileInfo, IModStore::ModLoadType modLoadType, QString updatedModId) {
    mThreadPoolExtractZips.start([this, fileInfo, modLoadType, updatedModId = std::move(updatedModId)] mutable {
        auto localInfo = getLocalInfoFromZip(fileInfo);

        if (localInfo.canConvert<ModEntry::LocalInfo>()) {
            QMetaObject::invokeMethod(
                this,
                [this, modInfo_ = std::move(localInfo).value<ModEntry::LocalInfo>(), modLoadType,
                 updatedModId] mutable {
                    // Copy id for info retrieval
                    QString modId = modInfo_.mId;

                    if (modLoadType == IModStore::ModLoadType::Update && modId != updatedModId) {
                        mStore->onModUpdateFailed(
                            updatedModId, u"Different updated mod got: %1 expected: %2"_s.arg(modId, updatedModId));
                        decrementModsLoadingInProgress();
                        return;
                    }

                    // Auto disable new version if old one is disabled
                    if (mGameMngr->getDisabledMods().contains(modId)) {
                        mGameMngr->addDisabledMod(modId, QString::fromStdString(modInfo_.mVersion.to_string()));
                    }

                    mStore->add(std::move(modInfo_), modLoadType);
                    mHttpClient->sendGet(
                        GetModUrlApi(mApiUrl, modId), this, ONLINE_CONTENT_TYPE,
                        [this, modId](QByteArray data) mutable {
                            if (QJsonObject modObj = createOnlineModEntry(data, modId); !modObj.isEmpty()) {
                                mStore->updateOnline(modId, std::move(modObj));
                            }
                            decrementModsLoadingInProgress();
                        },
                        [this, modId](QString error) {
                            qCWarning(cModLoader, "Error retrieving info for %s: %s", qUtf8Printable(modId),
                                      qUtf8Printable(std::move(error)));
                            decrementModsLoadingInProgress();
                        },
                        {});
                },
                Qt::QueuedConnection);
            return;
        }
        qCWarning(cModLoader, "%s", qUtf8Printable(std::move(localInfo).value<QString>()));
        if (modLoadType == IModStore::ModLoadType::Update) {
            reportUpdateFailed(std::move(updatedModId), u"Downloaded update is not a valid mod"_s);
            return;
        }
        QMetaObject::invokeMethod(this, [this] { decrementModsLoadingInProgress(); }, Qt::QueuedConnection);
    });
}

QVariant ModLoader::getLocalInfoFromZip(QFileInfo fileInfo) {
    const QString absoluteFilePath = fileInfo.absoluteFilePath();
    ZipArchive zipArchive(absoluteFilePath);

    if (const auto opened = zipArchive.open(); !opened) {
        return opened.error();
    }

    const auto zipFileId = zipArchive.getFileIndex("modinfo.json");
    if (!zipFileId) {
        return zipFileId.error();
    }

    const auto fileBuffer = zipArchive.getFileContent(*zipFileId);
    if (!fileBuffer) {
        return fileBuffer.error();
    }
    return parseLocalJson(*fileBuffer, std::move(fileInfo));
}

QVariant ModLoader::parseLocalJson(const QByteArray &jsonByteArray, QFileInfo &&fileInfo) {
    ModEntry::LocalInfo info;
    info.mFileInfo = std::move(fileInfo);

    QJsonParseError errorCode{.error = QJsonParseError::NoError};
    QJsonObject json = QJsonDocument::fromJson(jsonByteArray, &errorCode).object();
    if (errorCode.error != QJsonParseError::NoError) {
        return u"Failed to parse modinfo.json from zip file: %1 Reason: %2"_s.arg(info.mFileInfo.absoluteFilePath())
            .arg(errorCode.errorString());
    }
    for (const auto &[key, value] : json.asKeyValueRange()) {
        auto lowercaseKey = key.toString().toLower();

        if (lowercaseKey == LOCAL_JSON_VERSION_KEY) {
            if (const auto result = semver::parse(value.toString().toStdString(), info.mVersion); !result) {
                return u"Failed to parse modinfo.json from zip file: %1"_s.arg(info.mFileInfo.absoluteFilePath());
            }
        } else if (lowercaseKey == LOCAL_JSON_MODID_KEY && value.isString()) {
            info.mId = value.toString();
        } else if (lowercaseKey == LOCAL_JSON_NAME_KEY && value.isString()) {
            info.mName = value.toString();
        } else if (lowercaseKey == LOCAL_JSON_AUTHORS_KEY && value.isArray() && !value.toArray().isEmpty()) {
            info.mAuthor = value.toArray().at(0).toString();
        }
    }

    if (info.mId.isEmpty() || info.mAuthor.isEmpty() || info.mName.isEmpty()) {
        return u"Failed to parse modinfo.json from zip file: %1"_s.arg(info.mFileInfo.absoluteFilePath());
    }

    return QVariant::fromValue(std::move(info));
}

QJsonObject ModLoader::createOnlineModEntry(const QByteArray &jsonByteArray, QAnyStringView modId) {
    QJsonParseError parseError{.error = QJsonParseError::NoError};
    auto responseJsonObj = QJsonDocument::fromJson(jsonByteArray, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(cModLoader, "Failed to parse online info for %s: %s", qUtf8Printable(modId.toString()),
                  qUtf8Printable(parseError.errorString()));
        return {};
    }

    if (!responseJsonObj.isObject()) {
        qCWarning(cModLoader, "Retrieved invalid response for %s: not an object", qUtf8Printable(modId.toString()));
        return {};
    }

    auto responseObj = responseJsonObj.object();

    // extra check for statuscode in json response
    if (responseObj[ONLINE_STATUSCODE_JSON_KEY].toStringView() != "200"_L1) {
        qCInfo(cModLoader, "Cannot retrieve online info for %s: status %s", qUtf8Printable(modId.toString()),
               qUtf8Printable(responseObj[ONLINE_STATUSCODE_JSON_KEY].toString()));
        return {};
    }

    if (!responseObj[ONLINE_JSON_ROOT_KEY].isObject()) {
        qCWarning(cModLoader, "Retrieved invalid response for %s: no mod object", qUtf8Printable(modId.toString()));
        return {};
    }

    return responseObj[ONLINE_JSON_ROOT_KEY].toObject();
}
} // namespace vsmm