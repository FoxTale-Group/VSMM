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
#include <semver/semver.hpp>

namespace vsmm {

struct LocalModInfo {
    QString mName, mId, mAuthor;
    semver::version mVersion{};
    QFileInfo mFileInfo;
};

class MODENTRY_EXPORT ModEntry {
  public:
    explicit ModEntry(LocalModInfo info);

    [[nodiscard]] QAnyStringView getId() const;
    [[nodiscard]] QAnyStringView getName() const;
    [[nodiscard]] QAnyStringView getAuthor() const;
    [[nodiscard]] const semver::version &getVersion() const;
    [[nodiscard]] const QFileInfo &getFileInfo() const;

    void initOnlineInfo(QJsonObject json);

    // Online
    [[nodiscard]] const QUrl &getUrl() const;
    [[nodiscard]] const semver::version &getLatestVersion() const;
    [[nodiscard]] const QStringList &getTags() const;
    [[nodiscard]] const QUrl &getLatestVersionUrl() const;
    [[nodiscard]] QAnyStringView getType() const;
    [[nodiscard]] bool hasUpdate() const;

  private:
    struct OnlineInfo {
        QString mName, mAuthor, mType;
        QStringList mTags;
        QUrl mUrl, mLatestReleaseUrl;
        semver::version mLatestVersion;
    };

    void initName(const QJsonObject &json);
    void initLatestRelease(const QJsonObject &json);
    void initAuthor(const QJsonObject &json);
    void initTags(const QJsonObject &json);
    void initModUrl(const QJsonObject &json);
    void initType(const QJsonObject &json);

    QString mName;
    semver::version mVersion;
    QString mAuthor;
    QString mModId;
    QFileInfo mFileInfo;
    OnlineInfo mOnlineInfo;

    bool mHasUpdate{false};
};
} // namespace vsmm
