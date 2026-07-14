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

#include <ModEntryExport.hpp>
#include <QFileInfo>
#include <QJsonObject>
#include <semver.hpp>

namespace vsmm {

class MODENTRY_EXPORT ModEntry {
  public:
    struct LatestVersion {
        QUrl mUrl;
        semver::version<> mVersion;
        QString mFileName;
        QStringList mSupportedVersions;
        bool mHasUpdate{false};
    };
    struct LocalInfo {
        QString mName, mId, mAuthor;
        semver::version<> mVersion{};
        QFileInfo mFileInfo;
        [[nodiscard]] QString toString() const {
            return QStringLiteral("%1@%2").arg(mId).arg(QString::fromStdString(mVersion.to_string()));
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator QString() const { return toString(); }
    };

    explicit ModEntry(LocalInfo info);

    [[nodiscard]] QString toString() const;

    // ReSharper disable once CppNonExplicitConversionOperator
    operator QString() const { return toString(); }

    [[nodiscard]] QStringView getId() const;
    [[nodiscard]] QStringView getName() const;
    [[nodiscard]] QStringView getAuthor() const;
    [[nodiscard]] const semver::version<> &getVersion() const;
    [[nodiscard]] const QFileInfo &getFileInfo() const;
    [[nodiscard]] bool isMarkedForUpdate() const;
    [[nodiscard]] bool isFavorite() const;
    void setMarkedForUpdate(bool marked);
    void setFavorite(bool favorite);

    void initOnlineInfo(QJsonObject json, const semver::version<> &gameVersion, bool cfgIncludePrerelease);

    // Online
    [[nodiscard]] const QUrl &getUrl() const;
    [[nodiscard]] const LatestVersion &getLatestVersion() const;
    [[nodiscard]] const QStringList &getTags() const;
    [[nodiscard]] QStringView getType() const;
    [[nodiscard]] bool hasUpdate() const;

  private:
    struct OnlineInfo {
        QString mName, mAuthor, mType;
        QStringList mTags;
        QUrl mUrl;
        LatestVersion mLatestVersion;
    };

    void initName(const QJsonObject &json);
    void initLatestRelease(const QJsonObject &json, const semver::version<> &gameVersion, bool cfgIncludePrerelease);
    void initAuthor(const QJsonObject &json);
    void initTags(const QJsonObject &json);
    void initModUrl(const QJsonObject &json);
    void initType(const QJsonObject &json);

    [[nodiscard]] QPair<QJsonObject, semver::version<>>
    getLatestVersion(QJsonArray releases, const semver::version<> &gameVersion, bool cfgIncludePrerelease) const;

    QString mName;
    semver::version<> mVersion;
    QString mAuthor;
    QString mModId;
    QFileInfo mFileInfo;
    OnlineInfo mOnlineInfo;
    bool mMarkedForUpdate{false};
    bool mFavorite{false};
};
} // namespace vsmm
