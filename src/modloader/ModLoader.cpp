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
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

#include <algorithm>

#include <ZipArchive.hpp>
#include <semver.hpp>
#include <utility>

namespace {
using namespace Qt::StringLiterals;
QUrl GetModUrlApi(QAnyStringView modId) { return u"https://mods.vintagestory.at/api/mod/%1"_s.arg(modId); }
} // namespace

namespace vsmm {
ModLoader::ModLoader() {
    mThreadPoolExtractZips.setMaxThreadCount(std::min(4, QThread::idealThreadCount()));
    mThreadPoolProcessUpdate.setMaxThreadCount(std::min(4, QThread::idealThreadCount()));
}

bool ModLoader::initModsList() {
    static const QStringList modsExts{{"*.zip"}};

    if (!mGameMngr) {
        qFatal("GameMngr is not set");
    }

    QList<QDir> modsDirs = mGameMngr->getModsDirs();
    if (modsDirs.isEmpty()) {
        if (!mModsLoadingInProgress.loadRelaxed()) {
            emit allModsReloaded();
        }
        qCritical("No mods dirs found");
        return false;
    }

    for (const auto &modsDir : modsDirs) {
        if (!modsDir.exists()) {
            qWarning() << "Mods path " << modsDir.path() << " does not exist";
            continue;
        }

        for (auto &entry : modsDir.entryInfoList(modsExts, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
            load(std::move(entry));
        }
    }

    if (!mModsLoadingInProgress.loadRelaxed()) {
        emit allModsReloaded();
    }
    return true;
}

void ModLoader::setHttpClient(HttpClient *httpClient) { mHttpClient = httpClient; }
void ModLoader::setGameMngr(GameMngr *gameMngr) { mGameMngr = gameMngr; }

void ModLoader::setStore(ModStore *store) {
    mStore = store;
    connect(mStore, &ModStore::modsReloading, this, &ModLoader::onModsReloading);
    connect(mStore, &ModStore::modAddedFromGUI, this, &ModLoader::onLoadFromGUI);
    connect(mStore, &ModStore::modUpdateRequested, this, &ModLoader::onModUpdateRequested);
    connect(this, &ModLoader::allModsReloaded, mStore, &ModStore::onModsReloaded);
}

void ModLoader::load(QFileInfo &&fileInfo) {
    incrementModsLoadingInProgress();
    load_(std::move(fileInfo));
}
void ModLoader::onLoadFromGUI(const QUrl &filePath) {
    incrementModsLoadingInProgress();
    load_(QFileInfo{filePath.toLocalFile()});
}
void ModLoader::onModUpdateRequested(const ModEntry &mod) {
    if (!mod.hasUpdate()) {
        return;
    }

    incrementModsLoadingInProgress();

    ModEntry::LatestVersion latestVersion = mod.getLatestVersion();
    mHttpClient->sendGet(
        latestVersion.mUrl, this, DOWNLOAD_CONTENT_TYPE,
        [this, latestVersion](QByteArray data) mutable {
            onModUpdateRetrieved(std::move(data), std::move(latestVersion));
        },
        [this, id = mod.getId()](const QString &error) {
            qWarning() << u"Failed to retrieve update for %1: %2"_s.arg(id).arg(error);
            decrementModsLoadingInProgress();
        });
}

void ModLoader::onModIconRetrieved(QString modId, QByteArray data) {
    mThreadPoolProcessIcon.start([this, modId = std::move(modId), data = std::move(data)] mutable {
        QImage image;
        if (!image.loadFromData(data)) {
            qWarning() << u"Failed to retrieve mod %1 icon: Invalid image data"_s.arg(modId);
            return;
        }
        if (image.isNull()) {
            qWarning() << u"Failed to retrieve mod %1 icon: Empty image"_s.arg(modId);
            return;
        }
        QMetaObject::invokeMethod(
            this,
            [this, modId = std::move(modId), image_ = std::move(image)] mutable {
                emit modIconDownloaded(modId, std::move(image_));
            },
            Qt::QueuedConnection);
    });
}

void ModLoader::onModUpdateRetrieved(QByteArray data, ModEntry::LatestVersion latestVersion) {
    mThreadPoolProcessUpdate.start([this, latestVersion = std::move(latestVersion), data = std::move(data)] {
        QFile modUpdateFile{QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() +
                            latestVersion.mFileName};
        if (!modUpdateFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate)) {
            qWarning() << "Failed to create file for mod update";
            decrementModsLoadingInProgress();
            return;
        }
        if (modUpdateFile.write(data) != data.size()) {
            qWarning() << "Failed to write mod update data to file";
            decrementModsLoadingInProgress();
            return;
        }
        modUpdateFile.close();
        QFileInfo fileInfo{modUpdateFile.fileName()};
        QMetaObject::invokeMethod(this, [this, fileInfo] mutable { load_(std::move(fileInfo)); }, Qt::QueuedConnection);
    });
}

void ModLoader::incrementModsLoadingInProgress() { mModsLoadingInProgress.fetchAndAddRelaxed(1); }

void ModLoader::decrementModsLoadingInProgress() {
    if (mModsLoadingInProgress.fetchAndSubRelaxed(1) == 1) {
        QMetaObject::invokeMethod(this, [this] { emit allModsReloaded(); }, Qt::QueuedConnection);
    }
}

void ModLoader::onModsReloading() { initModsList(); }

void ModLoader::load_(QFileInfo &&fileInfo) {
    mThreadPoolExtractZips.start([this, fileInfo] mutable {
        auto localInfo = getLocalInfoFromZip(std::move(fileInfo));

        if (localInfo.isValid() && localInfo.canConvert<ModEntry::LocalInfo>()) {
            QMetaObject::invokeMethod(
                this,
                [this, modInfo_ = std::move(localInfo).value<ModEntry::LocalInfo>()] mutable {
                    // Copy id for info retrieval
                    QString modId = modInfo_.mId;
                    mStore->add(std::move(modInfo_));
                    mHttpClient->sendGet(
                        GetModUrlApi(modId), this, ONLINE_CONTENT_TYPE,
                        [this, modId](QByteArray data) mutable {
                            onModInfoRetrieved(std::move(modId), std::move(data));
                        },
                        [this](const QString &error) {
                            qWarning() << u"Error retrieving mod info: %1"_s.arg(error);
                            decrementModsLoadingInProgress();
                        });
                },
                Qt::QueuedConnection);
            return;
        }
        qWarning() << std::move(localInfo).value<QString>();
        QMetaObject::invokeMethod(this, [this] { decrementModsLoadingInProgress(); }, Qt::QueuedConnection);
    });
}

void ModLoader::onModInfoRetrieved(QString modId, QByteArray data) {
    QJsonObject modObj = createOnlineModEntry(std::move(data), modId);
    if (modObj.isEmpty()) {
        decrementModsLoadingInProgress();
        return;
    }

    if (modObj[ONLINE_LOGOFILE_JSON_KEY].isString()) {
        QString logoFile = modObj[ONLINE_LOGOFILE_JSON_KEY].toString();
        if (!logoFile.isEmpty()) {
            mHttpClient->sendGet(
                logoFile, this, "image/",
                [this, modId](QByteArray data) mutable { onModIconRetrieved(std::move(modId), std::move(data)); },
                [](const QString &error) { qWarning() << u"Error retrieving mod icon: %1"_s.arg(error); });
        }
    }
    mStore->updateOnline(modId, std::move(modObj));
    decrementModsLoadingInProgress();
}

QVariant ModLoader::getLocalInfoFromZip(QFileInfo &&fileInfo) {
    const QString absoluteFilePath = fileInfo.absoluteFilePath();
    ZipArchive zipArchive(absoluteFilePath);

    if (const auto [open, errCode] = zipArchive.open(); !open) {
        return u"Failed to open zip file: %1 {%2}"_s.arg(absoluteFilePath).arg(errCode);
    }

    ZipArchive::FileIndex zipFileId = zipArchive.getFileIndex("modinfo.json");
    if (zipFileId == -1) {
        return u"Failed to locate modinfo.json in zip file: %1"_s.arg(absoluteFilePath);
    }

    auto fileBuffer = zipArchive.getFileContent(zipFileId);
    if (fileBuffer.isEmpty()) {
        return u"Failed to read modinfo.json from zip file: %1"_s.arg(absoluteFilePath);
    }
    return parseLocalJson(fileBuffer, std::move(fileInfo));
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

QJsonObject ModLoader::createOnlineModEntry(QByteArray jsonByteArray, QAnyStringView modId) {
    QJsonParseError parseError{.error = QJsonParseError::NoError};
    auto responseJsonObj = QJsonDocument::fromJson(jsonByteArray, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << u"Failed to parse online info for: %1 Reason: %2"_s.arg(modId).arg(parseError.errorString());
        return {};
    }

    if (!responseJsonObj.isObject()) {
        qWarning() << u"Retrieved invalid response for %1"_s.arg(modId);
        return {};
    }

    auto responseObj = responseJsonObj.object();

    // extra check for statuscode in json response
    if (responseObj[ONLINE_STATUSCODE_JSON_KEY].toStringView() != "200"_L1) {
        qInfo() << u"Cannot retrieve online info for %1 Reason: %2"_s.arg(modId).arg(
            responseObj[ONLINE_STATUSCODE_JSON_KEY].toStringView());
        return {};
    }

    if (!responseObj[ONLINE_JSON_ROOT_KEY].isObject()) {
        qWarning() << u"Retrieved invalid response for %1"_s.arg(modId);
        return {};
    }

    return responseObj[ONLINE_JSON_ROOT_KEY].toObject();
}
} // namespace vsmm