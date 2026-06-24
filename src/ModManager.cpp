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

#include "ModManager.hpp"
#include "ZipArchive.hpp"

#include <QJsonObject>
#include <QNetworkReply>
#include <QJsonArray>
#include <QThreadPool>

#include <semver/semver.hpp>

namespace {
    QUrl GetModUrlApi(QAnyStringView modId) {
        return QString("https://mods.vintagestory.at/api/mod/%1").arg(modId);
    }
}

namespace vsmodchecker {
    bool ModManager::setModsPath(std::filesystem::path modsPath) {
        if (!std::filesystem::exists(modsPath)) {
            qWarning() << QString("Mods path does not exist: %1").arg(QString::fromStdString(modsPath.string()));
            return false;
        }

        mModsPath = std::move(modsPath);
        return true;
    }

    bool ModManager::initModsList() {
        if (mModsPath.empty()) {
            qCritical("Mods path is not set");
            return false;
        }

        for (const auto& entry : std::filesystem::directory_iterator(mModsPath)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".zip") {
                continue;
            }

            mThreadPoolExtractZips.start([this, entry] {
                ZipArchive zipArchive(entry.path());
                ZipArchive::FileIndex zipFileId = zipArchive.getFileIndex("modinfo.json");
                if (zipFileId == -1) {
                    qWarning() << QString("Failed to locate modinfo.json in zip file: %1").arg(QString::fromStdString(entry.path().string()));
                    return;
                }

                const auto& [fileBuffer, fileSize] = zipArchive.getFileContent(zipFileId);
                auto modInfo = parseModInfoJson(QByteArray{fileBuffer.get(), fileSize}, QString::fromStdString(entry.path().string()));

                if (modInfo.id.isEmpty()) {
                    return;
                }

                QMetaObject::invokeMethod(this, [this, modInfo_ = std::move(modInfo)] mutable {
                    retrieveInfoForMod(std::move(modInfo_));
                }, Qt::QueuedConnection);
            });
        }
        return true;
    }

    void ModManager::reloadMods() {
        if (mRequestCount > 0) {
            qWarning() << "Cannot reload mods while requests are in progress";
            return;
        }
        mModsList.clear();
        emit modsCleared();
        initModsList();
    }

    const QHash<QString, ModEntry> &ModManager::getModsList() const {
        return mModsList;
    }

    void ModManager::setNetworkManager(QNetworkAccessManager *networkManager) {
        mNetworkManager = networkManager;
    }

    void ModManager::setModImageProvider(ModImageProvider *modImageProvider) {
        mModImageProvider = modImageProvider;
    }

    int ModManager::updatesAvailable() const {
        int updatesAvailable{0};
        for (const auto& mod : mModsList) {
            if (mod.hasUpdate()) {
                updatesAvailable++;
            }
        }
        return updatesAvailable;
    }

    quint64 ModManager::installedModsCount() const {
        return mModsList.size();
    }

    void ModManager::retrieveInfoForMod(ModInfoZip info) {
        if (!mNetworkManager) {
            qCritical() << "Network manager is not set";
            return;
        }

        ++mRequestCount;
        QNetworkRequest request(GetModUrlApi(info.id));
        QNetworkReply *reply = mNetworkManager->get(request);
        reply->setProperty("modInfo", QVariant::fromValue(std::move(info)));
        connect(reply, &QNetworkReply::finished, this, &ModManager::requestInfoFinished);
    }

    void ModManager::retrieveModIcon(const QString &id, const QUrl &url) {
        if (!mNetworkManager) {
            qCritical() << "Network manager is not set";
            return;
        }

        if (mModImageProvider->hasImage(id)) {
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

                QMetaObject::invokeMethod(this, [this, id, image]() {
                    mModImageProvider->addImage(id, image);
                    emit thumbnailReady(id);
                }, Qt::QueuedConnection);
            });
        });
    }

    ModManager::ModInfoZip ModManager::parseModInfoJson(QByteArrayView jsonByteArray, const QString &filename) {
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

    void ModManager::requestInfoFinished() {
        --mRequestCount;
        auto response = qobject_cast<QNetworkReply *>(sender());
        if (!response) {
            return;
        }
        response->deleteLater();

        auto info = response->property("modInfo").value<ModInfoZip>();
        if (response->error() != QNetworkReply::NoError) {
            qWarning() << QString("Failed to retrieve mod info for %1: %2").arg(info.id, response->errorString());
            QString id = info.id;
            const auto it = mModsList.emplace(std::move(id), std::move(info.name), std::move(info.version), std::move(info.author), std::move(info.id), std::move(info.filename));
            emit modEntryAdded(*it);
            return;
        }

        if (auto value = response->header(QNetworkRequest::ContentTypeHeader).toString(); value != "application/json") {
            qWarning() << QString("Invalid response format for %1: %2").arg(info.id, value);
            return;
        }

        const QByteArray responseData = response->readAll();
        auto responseJsonObj = QJsonDocument::fromJson(responseData).object()["mod"].toObject();

        if (auto [it, added] = mModsList.tryEmplace(info.id, responseJsonObj, info.version, info.id, info.filename); !added) {
            qWarning() << QString("Detected doubled mod %1. Checking version...").arg(info.name);

            try {
                if (semver::version::parse(it->getVersion().toString().toStdString()) <
                semver::version::parse(info.version.toStdString())) {
                    it = mModsList.emplace(info.id, responseJsonObj, info.version, info.id, info.filename);
                    emit modEntryUpdated(*it);

                    qInfo() << QString("Found newer version of %1. Overwriting...").arg(info.name);
                }
            } catch (const semver::semver_exception& e) {
                qWarning() << QString("Failed to parse other version for mod %1: %2").arg(info.name, e.what());
            }
        } else {
            retrieveModIcon(info.id, responseJsonObj["logofile"].toString());
            emit modEntryAdded(*it);
        }
    }
} // vsmodchecker