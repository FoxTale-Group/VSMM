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

namespace {
    QUrl GetModUrlApi(QAnyStringView modId) {
        return QString("https://mods.vintagestory.at/api/mod/%1").arg(modId);
    }
}

namespace vsmodchecker {
    void ModManager::setModsPath(std::filesystem::path modsPath) {
        if (!std::filesystem::exists(modsPath)) {
            throw std::runtime_error("Mods path does not exist");
        }

        mModsPath = std::move(modsPath);
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

            ZipArchive zipArchive(entry.path());
            ZipArchive::FileIndex zipFileId = zipArchive.getFileIndex("modinfo.json");
            if (zipFileId == -1) {
                qWarning() << QString("Failed to locate modinfo.json in zip file: %1").arg(QString::fromStdString(entry.path().string()));
                continue;
            }

            const auto& [fileBuffer, fileSize] = zipArchive.getFileContent(zipFileId);
            auto modInfo = parseModInfoJson(QByteArray{fileBuffer.get(), fileSize}, QString::fromStdString(entry.path().string()));

            if (modInfo.id.isEmpty()) {
                continue;
            }

            ++mRequestCount;
            retrieveInfoForMod(std::move(modInfo));
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

    void ModManager::retrieveInfoForMod(ModInfoZip info) {
        if (!mNetworkManager) {
            qCritical() << "Network manager is not set";
            return;
        }

        QNetworkRequest request(GetModUrlApi(info.id));
        QNetworkReply *reply = mNetworkManager->get(request);
        reply->setProperty("modInfo", QVariant::fromValue(std::move(info)));
        connect(reply, &QNetworkReply::finished, this, &ModManager::requestInfoFinished);
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
        auto response = qobject_cast<QNetworkReply *>(sender());
        if (!response) {
            return;
        }

        auto info = response->property("modInfo").value<ModInfoZip>();
        if (response->error() != QNetworkReply::NoError) {
            qWarning() << QString("Failed to retrieve mod info for %1: %2").arg(info.id, response->errorString());
            mModsList.emplace(info.id, std::move(info.name), std::move(info.version), std::move(info.author), std::move(info.id), std::move(info.filename));
            response->deleteLater();
            return;
        }

        if (auto value = response->header(QNetworkRequest::ContentTypeHeader).toString(); value != "application/json") {
            qWarning() << QString("Invalid response format for %1: %2").arg(info.id, value);
            response->deleteLater();
            return;
        }

        const QByteArray responseData = response->readAll();
        response->deleteLater();

        auto responseJsonObj = QJsonDocument::fromJson(responseData).object()["mod"].toObject();
        const auto it = mModsList.emplace(info.id, responseJsonObj, info.version, info.id, info.filename);

        --mRequestCount;
        emit modAdded(*it);
    }
} // vsmodchecker