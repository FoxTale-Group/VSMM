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
#include "ZipArchive.hpp"
#include "constants.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QThreadPool>

#include <semver/semver.hpp>
#include <utility>

namespace {
using namespace Qt::StringLiterals;
QUrl GetModUrlApi(QAnyStringView modId) { return u"https://mods.vintagestory.at/api/mod/%1"_s.arg(modId); }
} // namespace

namespace vsmm {
bool ModLoader::initModsList() {
    static const QStringList modsExts{{"*.zip"}};

    if (!mConfig) {
        qFatal("Config is not set");
        return false;
    }

    if (!mConfig->isReady()) {
        qCritical("Config is not ready");
        return false;
    }

    QList<QDir> modsDirs = mConfig->getModsDirs();
    if (modsDirs.isEmpty()) {
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

    if (mRequestCount == 0) {
        emit allModsReloaded();
    }
    return true;
}

void ModLoader::setNetworkManager(QNetworkAccessManager *networkManager) { mNetworkManager = networkManager; }
void ModLoader::setConfig(Config *config) { mConfig = config; }

void ModLoader::setStore(ModStore *store) {
    mStore = store;
    connect(mStore, &ModStore::modsReloading, this, &ModLoader::onModsReloading);
    connect(mStore, &ModStore::modAddedFromGUI, this, &ModLoader::onLoadFromGUI);
    connect(this, &ModLoader::allModsReloaded, mStore, &ModStore::onModsReloaded);
}

void ModLoader::load(QFileInfo &&fileInfo) { load_(std::move(fileInfo)); }
void ModLoader::onLoadFromGUI(const QUrl &filePath) { load_(QFileInfo{filePath.toLocalFile()}); }
void ModLoader::onModsReloading() { initModsList(); }

void ModLoader::notifyModProcessed() {
    if (--mRequestCount <= 0) {
        emit allModsReloaded();
    }
}

void ModLoader::retrieveInfoForMod(QString modId) {
    if (!mNetworkManager) {
        qCritical() << "Network manager is not set";
        notifyModProcessed();
        return;
    }

    QNetworkRequest request = createRequest(GetModUrlApi(modId));
    QNetworkReply *reply = mNetworkManager->get(request);
    reply->setProperty("modId", std::move(modId));
    connect(reply, &QNetworkReply::finished, this, &ModLoader::requestInfoFinished);
}

void ModLoader::retrieveModIcon(const QString &id, const QUrl &url) {
    using namespace Qt::StringLiterals;

    if (!mNetworkManager) {
        qCritical() << "Network manager is not set";
        return;
    }

    QNetworkRequest request = createRequest(url);
    QNetworkReply *reply = mNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, id] {
        auto *reply_ = qobject_cast<QNetworkReply *>(sender());

        reply_->deleteLater();
        if (reply_->error() != QNetworkReply::NoError) {
            qWarning() << u"Failed to retrieve mod %1 icon: %2"_s.arg(id).arg(reply_->errorString());
            return;
        }

        QByteArray imageData = reply_->readAll();
        QThreadPool::globalInstance()->start([this, id, imageData_ = std::move(imageData)] {
            QImage image;
            if (!image.loadFromData(imageData_)) {
                return;
            }

            QMetaObject::invokeMethod(
                this, [this, id, image_ = std::move(image)] mutable { emit modIconDownloaded(id, std::move(image_)); },
                Qt::QueuedConnection);
        });
    });
}

void ModLoader::load_(QFileInfo &&fileInfo) {
    using namespace Qt::StringLiterals;
    ++mRequestCount;
    mThreadPoolExtractZips.start([this, fileInfo_ = std::move(fileInfo)] mutable {
        QString absoluteFilePath = fileInfo_.absoluteFilePath();
        ZipArchive zipArchive(absoluteFilePath);

        if (const auto [open, errCode] = zipArchive.open(); !open) {
            qWarning() << u"Failed to open zip file: %1 {%2}"_s.arg(absoluteFilePath).arg(errCode);
            QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
            return;
        }

        ZipArchive::FileIndex zipFileId = zipArchive.getFileIndex("modinfo.json");
        if (zipFileId == -1) {
            qWarning() << u"Failed to locate modinfo.json in zip file: %1"_s.arg(absoluteFilePath);
            QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
            return;
        }

        auto fileBuffer = zipArchive.getFileContent(zipFileId);
        if (fileBuffer.isEmpty()) {
            qWarning() << u"Failed to read modinfo.json from zip file: %1"_s.arg(absoluteFilePath);
            QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
            return;
        }

        auto modInfo = parseLocalJson(fileBuffer, std::move(fileInfo_));
        if (modInfo.mId.isEmpty() || modInfo.mVersion == semver::version{} || modInfo.mAuthor.isEmpty() ||
            modInfo.mName.isEmpty()) {
            QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
            qWarning() << u"Failed to parse modinfo.json from zip file: %1"_s.arg(absoluteFilePath);
            return;
        }

        QMetaObject::invokeMethod(
            this,
            [this, modInfo_ = std::move(modInfo)] mutable {
                // Copy id for info retrieval
                QString modId = modInfo_.mId;
                mStore->add(std::move(modInfo_));
                retrieveInfoForMod(modId);
            },
            Qt::QueuedConnection);
    });
}

QNetworkRequest ModLoader::createRequest(const QUrl &url) {
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,
                      QStringLiteral("%1/%2").arg(APP_NAME).arg(APP_VERSION));
    return request;
}

void ModLoader::requestInfoFinished() {
    using namespace Qt::StringLiterals;

    notifyModProcessed();
    auto response = qobject_cast<QNetworkReply *>(sender());
    if (!response) {
        return;
    }
    response->deleteLater();

    const auto modId = response->property("modId").value<QString>();
    if (response->error() != QNetworkReply::NoError) {
        qWarning() << u"Failed to retrieve mod info for %1: %2"_s.arg(modId).arg(response->errorString());
        return;
    }

    if (auto value = response->header(QNetworkRequest::ContentTypeHeader).toString(); value != "application/json") {
        qWarning() << u"Invalid response format for %1: %2"_s.arg(modId).arg(value);
        return;
    }

    const QByteArray responseData = response->readAll();
    QJsonObject modObj = createOnlineModEntry(responseData, modId);
    if (modId.isEmpty()) {
        return;
    }

    if (modObj["logofile"_L1].isString()) {
        QString logoFile = modObj["logofile"_L1].toString();
        if (!logoFile.isEmpty()) {
            retrieveModIcon(modId, logoFile);
        }
    }
    mStore->updateOnline(modId, std::move(modObj));
}

LocalModInfo ModLoader::parseLocalJson(const QByteArray &jsonByteArray, QFileInfo &&fileInfo) {
    using namespace Qt::StringLiterals;

    LocalModInfo info;
    info.mFileInfo = std::move(fileInfo);

    QJsonParseError errorCode{.error = QJsonParseError::NoError};
    QJsonObject json = QJsonDocument::fromJson(jsonByteArray, &errorCode).object();
    if (errorCode.error != QJsonParseError::NoError) {
        qWarning() << u"Failed to parse modinfo.json from zip file: %1 Reason: %2"_s
                          .arg(info.mFileInfo.absoluteFilePath())
                          .arg(errorCode.errorString());
        return {};
    }
    for (const auto &[key, value] : json.asKeyValueRange()) {
        auto lowercaseKey = key.toString().toLower();

        if (lowercaseKey == "version" && value.isString()) {
            info.mVersion = semver::version::parse(value.toString().toStdString());
        } else if (lowercaseKey == "modid" && value.isString()) {
            info.mId = value.toString();
        } else if (lowercaseKey == "name" && value.isString()) {
            info.mName = value.toString();
        } else if (lowercaseKey == "authors" && value.isArray() && !value.toArray().isEmpty()) {
            info.mAuthor = value.toArray().at(0).toString();
        }
    }

    return info;
}

QJsonObject ModLoader::createOnlineModEntry(const QByteArray &jsonByteArray, QAnyStringView modId) {
    using namespace Qt::StringLiterals;

    QJsonParseError parseError{.error = QJsonParseError::NoError};
    auto responseJsonObj = QJsonDocument::fromJson(jsonByteArray, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << u"Failed to parse online info for: %1 Reason: %2"_s.arg(modId).arg(parseError.errorString());
        return {};
    }

    if (!responseJsonObj.isObject()) {
        qWarning() << u"Retreived invalid response for %1"_s.arg(modId);
        return {};
    }

    auto responseObj = responseJsonObj.object();
    if (!responseObj["mod"_L1].isObject()) {
        qWarning() << u"Retreived invalid response for %1"_s.arg(modId);
        return {};
    }

    return responseObj["mod"_L1].toObject();
}
} // namespace vsmm