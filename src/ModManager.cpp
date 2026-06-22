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

#include <semver/semver.hpp>

namespace {
    constexpr QUrl GetModUrl(QAnyStringView modId) {
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
            QString modVersion, modId;
            try {
                QJsonParseError errorCode{.error = QJsonParseError::NoError};
                QJsonObject json = QJsonDocument::fromJson(QByteArray{fileBuffer.get(), fileSize}, &errorCode).object();
                if (errorCode.error != QJsonParseError::NoError) {
                    qWarning() << QString("Failed to parse modinfo.json from zip file: %1 Reason: %2").arg(QString::fromStdString(entry.path().string()), errorCode.errorString());
                    continue;
                }
                for (const auto& [key, value] : json.asKeyValueRange()) {
                    auto lowercaseKey = key.toString().toLower();

                    if (lowercaseKey == "version") {
                        modVersion = value.toString();
                    } else if (lowercaseKey == "modid") {
                        modId = value.toString();
                    }
                }
            } catch (const std::exception& e) {
                qWarning() << QString("Failed to parse modinfo.json from zip file: %1 Reason: %2").arg(QString::fromStdString(entry.path().string()), e.what());
                continue;
            }

            ModEntry modEntry{
                .version = std::move(modVersion),
                .modid = std::move(modId),
                .filename = QString::fromStdString(entry.path().filename().string()),
            };

            mModsList.append(std::move(modEntry));
        }
        checkNewVersions();
        return true;
    }

    void ModManager::checkNewVersions() {
        if (!mNetworkManager) {
            qCritical() << "Network manager is not set";
            return;
        }

        for (auto& mod : mModsList) {
            QNetworkRequest request(GetModUrl(mod.modid));
            QNetworkReply *reply = mNetworkManager->get(request);
            reply->setProperty("modid", mod.modid);
            connect(reply, &QNetworkReply::finished, this, &ModManager::requestInfoFinished);
        }
    }

    void ModManager::reloadMods() {
        mModsList.clear();
        emit modsCleared();
        initModsList();
    }

    const QList<ModEntry> &ModManager::getModsList() const {
        return mModsList;
    }

    void ModManager::setNetworkManager(QNetworkAccessManager *networkManager) {
        mNetworkManager = networkManager;
    }

    void ModManager::requestInfoFinished() {
        auto response = qobject_cast<QNetworkReply *>(sender());
        if (!response) {
            return;
        }

        response->deleteLater();
        const QString modid = response->property("modid").toString();

        if (response->error() != QNetworkReply::NoError) {
            qWarning() << QString("Failed to retrieve mod info for %1: %2").arg(modid, response->errorString());
            return;
        }

        if (auto value = response->header(QNetworkRequest::ContentTypeHeader).toString(); value != "application/json") {
            qWarning() << QString("Invalid response format for %1: %2").arg(modid, value);
            return;
        }
        auto it = std::find_if(mModsList.begin(), mModsList.end(), [modid](const ModEntry& mod) {
            return mod.modid == modid;
        });

        if (it == mModsList.end()) {
            qWarning() << QString("Received reply for unknown modid: %1").arg(modid);
            return;
        }
        auto& mod = *it;

        const QByteArray responseData = response->readAll();
        auto responseJsonObj = QJsonDocument::fromJson(responseData).object()["mod"].toObject();

        auto modName = responseJsonObj["name"].toString();
        auto lastestReleaseJsonObj = responseJsonObj["releases"].toArray().first();

        mod.name = std::move(modName);
        mod.updateVersion = lastestReleaseJsonObj["modversion"].toString();
        mod.author = responseJsonObj["author"].toString();

        mod.tags.clear();
        for (const auto& tag : responseJsonObj["tags"].toArray()) {
            mod.tags.append(tag.toString());
        }

        qDebug() << QString("Retrieved mod info for %1").arg(mod.name);
        auto latestReleaseVersion = semver::version::parse( mod.updateVersion.toStdString());
        auto currentVersion = semver::version::parse(mod.version.toStdString());
        if (latestReleaseVersion > currentVersion) {
            qInfo() << QString("New version available for %1: %2").arg(mod.name).arg(mod.updateVersion);
        } else {
            mod.updateVersion = "latest";
            qInfo() << QString("No new version available for %1").arg(mod.name);
        }

        emit modAdded(mod);
    }
} // vsmodchecker