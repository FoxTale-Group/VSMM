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

#include <QByteArray>
#include <QDateTime>
#include <QLatin1StringView>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <optional>

namespace vsmm::modinfocache {
struct ModListEntry {
    qint64 mModId{}, mAssetId{}, mDownloads{}, mFollows{}, mComments{}, mTrendingPoints{};
    QString mName, mSummary, mAuthor, mUrlAlias, mSide, mType;
    QUrl mLogo;
    QDateTime mLastReleased;
    QStringList mModIdStrs, mTags;
};

[[nodiscard]] std::optional<QList<ModListEntry>> parseModList(const QByteArray &modList);
[[nodiscard]] bool writeModList(QLatin1StringView connection, const QString &dbPath,
                                const QList<ModListEntry> &entries);
} // namespace vsmm::modinfocache
