/*
 * VS Mod Manager - A mod management tool for Vintage Story
 * Copyright (C) 2026 Amaroq & StardustVulpine
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

#include <QJsonObject>
#include <QNetworkReply>
#include <QJsonArray>
#include <QThreadPool>

#include <utility>
#include <semver/semver.hpp>

namespace {
    QUrl GetModUrlApi(QAnyStringView modId) {
        return QString("https://mods.vintagestory.at/api/mod/%1").arg(modId);
    }
}

namespace vsmodchecker {
    bool ModLoader::setModsPath(const QString& modsPath) {
        QDir modsDir = modsPath;
        if (!modsDir.exists()) {
            qWarning() << QString("Mods path does not exist: %1").arg(modsPath);
            return false;
        }

        mModsPath = std::move(modsDir);
        return true;
    }

    bool ModLoader::initModsList() {
        if (mModsPath.path().isEmpty()) {
            qCritical("Mods path is not set");
            return false;
        }

        const auto entryList = mModsPath.entryInfoList(QStringList{"*.zip"}, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
        for (const auto& entry : entryList) {
            load(entry);
        }

        if (mRequestCount == 0) {
            emit allModsReloaded();
        }
        return true;
    }

    void ModLoader::setNetworkManager(QNetworkAccessManager *networkManager) {
        mNetworkManager = networkManager;
    }

    void ModLoader::setStore(ModStore *store) {
        mStore = store;
        connect(mStore, &ModStore::modsReloading, this, &ModLoader::onModsReloading);
        connect(mStore, &ModStore::modAddedFromGUI, this, &ModLoader::onLoadFromGUI);
        connect(this, &ModLoader::allModsReloaded, mStore, &ModStore::onModsReloaded);
    }

    void ModLoader::load(const QFileInfo &fileInfo) {
        load(fileInfo.absoluteFilePath(), false);
    }

    void ModLoader::onLoadFromGUI(const QString &filePath) {
        load(QUrl{filePath}.toLocalFile(), true);
    }

    void ModLoader::onModsReloading() {
        initModsList();
    }

    void ModLoader::notifyModProcessed() {
        if (--mRequestCount <= 0) {
            emit allModsReloaded();
        }
    }

    void ModLoader::retrieveInfoForMod(ModInfoZip info) {
        if (!mNetworkManager) {
            qCritical() << "Network manager is not set";
            notifyModProcessed();
            return;
        }

        QNetworkRequest request(GetModUrlApi(info.id));
        QNetworkReply *reply = mNetworkManager->get(request);
        reply->setProperty("modInfo", QVariant::fromValue(std::move(info)));
        connect(reply, &QNetworkReply::finished, this, &ModLoader::requestInfoFinished);
    }

    void ModLoader::retrieveModIcon(const QString &id, const QUrl &url) {
        if (!mNetworkManager) {
            qCritical() << "Network manager is not set";
            return;
        }

        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

        QNetworkReply *reply = mNetworkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, id] {
            auto *reply_ = qobject_cast<QNetworkReply *>(sender());

            reply_->deleteLater();
            if (reply_->error() != QNetworkReply::NoError) {
                qWarning() << QString("Failed to retrieve mod %1 icon: %2").arg(id).arg(reply_->errorString());
                return;
            }

            QByteArray imageData = reply_->readAll();

            QThreadPool::globalInstance()->start([this, id, imageData] {
                QImage image;
                if (!image.loadFromData(imageData)) {
                    return;
                }

                QMetaObject::invokeMethod(this, [this, id, image_ = std::move(image)] mutable {
                    emit modIconDownloaded(id, std::move(image_));
                }, Qt::QueuedConnection);
            });
        });
    }

    void ModLoader::load(const QString& filePath, bool moveToModsDir) {
        ++mRequestCount;
        mThreadPoolExtractZips.start([this, filePath, moveToModsDir] mutable {
            ZipArchive zipArchive(filePath);

            if (const auto [open, errCode] = zipArchive.open(); !open) {
                qWarning() << QString("Failed to open zip file: %1 {%2}").arg(filePath).arg(errCode);
                QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
                return;
            }

            ZipArchive::FileIndex zipFileId = zipArchive.getFileIndex("modinfo.json");
            if (zipFileId == -1) {
                qWarning() << QString("Failed to locate modinfo.json in zip file: %1").arg(filePath);
                QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
                return;
            }

            auto fileBuffer = zipArchive.getFileContent(zipFileId);
            if (fileBuffer.isEmpty()) {
                qWarning() << QString("Failed to read modinfo.json from zip file: %1").arg(filePath);
                QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
                return;
            }

            auto modInfo = parseModInfoJson(fileBuffer, filePath);
            if (modInfo.id.isEmpty()) {
                QMetaObject::invokeMethod(this, [this] { notifyModProcessed(); }, Qt::QueuedConnection);
                return;
            }

            QMetaObject::invokeMethod(this, [this, modInfo_ = std::move(modInfo), moveToModsDir, filePath] mutable {
                if (moveToModsDir) {
                    QFile::copy(filePath, mModsPath.absolutePath() + QDir::separator() + QFileInfo{filePath}.fileName());
                }
                retrieveInfoForMod(std::move(modInfo_));
            }, Qt::QueuedConnection);
        });
    }

    ModLoader::ModInfoZip ModLoader::parseModInfoJson(QByteArrayView jsonByteArray, const QString &filename) {
        ModInfoZip info;
        QJsonParseError errorCode{.error = QJsonParseError::NoError};
        QJsonObject json = QJsonDocument::fromJson(jsonByteArray.toByteArray(), &errorCode).object();
        if (errorCode.error != QJsonParseError::NoError) {
            qWarning() << QString("Failed to parse modinfo.json from zip file: %1 Reason: %2").arg(filename).arg(errorCode.errorString());
            return {};
        }
        for (const auto& [key, value] : json.asKeyValueRange()) {
            auto lowercaseKey = key.toString().toLower();

            if (lowercaseKey == "version") {
                info.version = value.toString();
            } else if (lowercaseKey == "modid") {
                info.id = value.toString();
            } else if (lowercaseKey == "name") {
                info.name = value.toString();
            } else if (lowercaseKey == "authors") {
                info.author = value.toArray().at(0).toString();
            }
        }

        return info;
    }

    void ModLoader::requestInfoFinished() {
        notifyModProcessed();
        auto response = qobject_cast<QNetworkReply *>(sender());
        if (!response) {
            return;
        }
        response->deleteLater();

        auto info = response->property("modInfo").value<ModInfoZip>();
        if (response->error() != QNetworkReply::NoError) {
            qWarning() << QString("Failed to retrieve mod info for %1: %2").arg(info.id, response->errorString());
            if (!mStore->contains(info.id)) {
                mStore->add(ModEntry{info.name, info.version, info.author, info.id, info.filename});
            }
            return;
        }

        if (auto value = response->header(QNetworkRequest::ContentTypeHeader).toString(); value != "application/json") {
            qWarning() << QString("Invalid response format for %1: %2").arg(info.id, value);
            return;
        }

        const QByteArray responseData = response->readAll();
        auto responseJsonObj = QJsonDocument::fromJson(responseData).object()["mod"].toObject();

        if (const ModEntry* existing = mStore->find(info.id)) {
            qWarning() << QString("Detected doubled mod %1. Checking version...").arg(info.name);

            try {
                if (semver::version::parse(existing->getVersion().toString().toStdString()) <
                    semver::version::parse(info.version.toStdString())) {
                    mStore->replace(ModEntry{responseJsonObj, info.version, info.id, info.filename});
                    qInfo() << QString("Found newer version of %1. Overwriting...").arg(info.name);
                }
            } catch (const semver::semver_exception& e) {
                qWarning() << QString("Failed to parse other version for mod %1: %2").arg(info.name, e.what());
            }
        } else {
            retrieveModIcon(info.id, responseJsonObj["logofile"].toString());
            mStore->add(ModEntry{responseJsonObj, info.version, info.id, info.filename});
        }
    }
} // vsmodchecker