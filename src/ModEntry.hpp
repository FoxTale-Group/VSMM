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

#pragma once

#include <QJsonObject>

namespace vsmodchecker {
class ModEntry {
  public:
    ModEntry(const QJsonObject &json, QString version, QString modId, QString filename);
    ModEntry(QString name, QString version, QString author, QString modId, QString filename);

    [[nodiscard]] QAnyStringView getId() const;
    [[nodiscard]] QAnyStringView getName() const;
    [[nodiscard]] QAnyStringView getAuthor() const;
    [[nodiscard]] QAnyStringView getVersion() const;
    [[nodiscard]] const QUrl &getUrl() const;
    [[nodiscard]] QAnyStringView getUpdateVersion() const;
    [[nodiscard]] const QStringList &getTags() const;
    [[nodiscard]] const QUrl &getLatestVersionUrl() const;
    [[nodiscard]] QAnyStringView getType() const;
    [[nodiscard]] bool hasUpdate() const;
    [[nodiscard]] bool hasInfoReceived() const;

  private:
    void initName(const QJsonObject &json);
    void initUpdateVersion(const QJsonObject &json);
    void initAuthor(const QJsonObject &json);
    void initTags(const QJsonObject &json);
    void initModUrl(const QJsonObject &json);
    void initType(const QJsonObject &json);

    QString mName;
    QString mVersion;
    QString mAuthor;
    QString mModId;
    QString mFilename;
    QString mUpdateVersion;
    QUrl mLatestVersionUrl;
    QStringList mTags;
    QUrl mUrl;
    QString mType;

    bool mHasInfoReceived{false};
    bool mHasUpdate{false};
};
} // namespace vsmodchecker
