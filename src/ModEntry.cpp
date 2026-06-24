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

#include "ModEntry.hpp"
#include <QJsonArray>
#include <utility>

#include <semver/semver.hpp>

namespace {
    QUrl GetModUrlByAlias(QAnyStringView alias) {
        return QString("https://mods.vintagestory.at/%1").arg(alias);
    }

    QUrl GetModUrlByAssetId(qint64 assetId) {
        return QString("https://mods.vintagestory.at/show/mod/%1").arg(assetId);
    }
}

namespace vsmodchecker {
    ModEntry::ModEntry(const QJsonObject& json, QString version, QString modId, QString filename) :
    mVersion{std::move(version)},
    mModId{std::move(modId)},
    mFilename{std::move(filename)},
    mHasInfoReceived{true}
    {
        initName(json);
        initModUrl(json);
        initAuthor(json);
        initTags(json);
        initUpdateVersion(json);
        initType(json);

        qDebug() << QString("Retrieved mod info for %1").arg(mName);
    }

    ModEntry::ModEntry(QString name, QString version, QString author, QString modId, QString filename) :
        mName(std::move(name)),
        mVersion(std::move(version)),
        mAuthor(std::move(author)),
        mModId(std::move(modId)),
        mFilename(std::move(filename))
    {
    }

    QAnyStringView ModEntry::getId() const {
        return mModId;
    }

    QAnyStringView ModEntry::getName() const {
        return mName;
    }

    QAnyStringView ModEntry::getAuthor() const {
        return mAuthor;
    }

    QAnyStringView ModEntry::getVersion() const {
        return mVersion;
    }

    const QUrl& ModEntry::getUrl() const {
        return mUrl;
    }

    QAnyStringView ModEntry::getUpdateVersion() const {
        return mUpdateVersion;
    }

    const QStringList& ModEntry::getTags() const {
        return mTags;
    }

    const QUrl& ModEntry::getLatestVersionUrl() const {
        return mLatestVersionUrl;
    }

    QAnyStringView ModEntry::getType() const {
        return mType;
    }

    bool ModEntry::hasUpdate() const {
        return mHasUpdate;
    }

    bool ModEntry::hasInfoReceived() const {
        return mHasInfoReceived;
    }

    void ModEntry::initName(const QJsonObject &json) {
        mName = json["name"].toString().trimmed();
    }

    void ModEntry::initUpdateVersion(const QJsonObject &json) {
        if (!json["releases"].isArray()) {
            auto errMsg = QString("%1: Invalid JSON format: releases is not an array").arg(mName);
            qCritical() << errMsg;
            return;
        }

        auto jsonReleaseArr = json["releases"].toArray();
        if (jsonReleaseArr.isEmpty()) {
            qWarning() << QString("%1: Invalid JSON format: releases array is empty").arg(mName);
            return;
        }

        auto latestReleaseObj = jsonReleaseArr.first().toObject();
        try {
            semver::version latestReleaseVersion = semver::version::parse(latestReleaseObj["modversion"].toString().toStdString());
            semver::version currentVersion = semver::version::parse(mVersion.toStdString());

            if (latestReleaseVersion > currentVersion) {
                mUpdateVersion = latestReleaseObj["modversion"].toString();
                mHasUpdate = true;
            }
        } catch (const semver::semver_exception &e) {
            qCritical() << QString("%1: Cannot parse version: %2").arg(mName).arg(e.what());
        }
    }

    void ModEntry::initAuthor(const QJsonObject &json) {
        mAuthor = json["author"].toString();
    }

    void ModEntry::initTags(const QJsonObject &json) {
        mTags.clear();
        for (const auto& tag : json["tags"].toArray()) {
            if (tag.isNull()) {
                continue;
            }
            mTags.append(tag.toString());
        }
    }

    void ModEntry::initModUrl(const QJsonObject &json) {
        if (!json["urlalias"].isNull()) {
            mUrl = GetModUrlByAlias(json["urlalias"].toString());
            return;
        }
        mUrl = GetModUrlByAssetId(json["assetid"].toInteger());
    }

    void ModEntry::initType(const QJsonObject &json) {
        mType = json["type"].toString();
    }
}
