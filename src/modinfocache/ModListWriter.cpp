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

#include "ModListWriter.hpp"
#include "ModInfoCacheValues.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

Q_STATIC_LOGGING_CATEGORY(cModInfoCache, "modinfocache");

using namespace Qt::StringLiterals;

namespace vsmm::modinfocache {
std::optional<QList<ModListEntry>> parseModList(const QByteArray &modList) {
    QJsonParseError parseError{.error = QJsonParseError::NoError};
    const QJsonObject root = QJsonDocument::fromJson(modList, &parseError).object();
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(cModInfoCache, "Cannot parse the mod list: %s", qUtf8Printable(parseError.errorString()));
        return std::nullopt;
    }
    if (root["statuscode"_L1].toStringView() != "200"_L1 || !root["mods"_L1].isArray()) {
        qCWarning(cModInfoCache, "Mod list reply has status %s and no mods array",
                  qUtf8Printable(root["statuscode"_L1].toString()));
        return std::nullopt;
    }
    const QJsonArray mods = root["mods"_L1].toArray();
    // dont store empty response
    if (mods.isEmpty()) {
        qCWarning(cModInfoCache, "Mod list reply has no mods, keeping the stored list");
        return std::nullopt;
    }

    QList<ModListEntry> entries;
    entries.reserve(mods.size());
    for (const auto &value : mods) {
        const QJsonObject mod = value.toObject();
        if (!mod["modid"_L1].isDouble()) {
            qCWarning(cModInfoCache, "Skipped mod list entry %s without modid",
                      qUtf8Printable(mod["name"_L1].toString()));
            continue;
        }
        ModListEntry entry;
        entry.mModId = mod["modid"_L1].toInteger();
        entry.mAssetId = mod["assetid"_L1].toInteger();
        entry.mDownloads = mod["downloads"_L1].toInteger();
        entry.mFollows = mod["follows"_L1].toInteger();
        entry.mComments = mod["comments"_L1].toInteger();
        entry.mTrendingPoints = mod["trendingpoints"_L1].toInteger();
        entry.mName = mod["name"_L1].toString().trimmed();
        entry.mSummary = mod["summary"_L1].toString().trimmed();
        entry.mAuthor = mod["author"_L1].toString().trimmed();
        entry.mUrlAlias = mod["urlalias"_L1].toString().trimmed();
        entry.mSide = mod["side"_L1].toString();
        entry.mType = mod["type"_L1].toString();
        entry.mLogo = QUrl{mod["logo"_L1].toString()};
        // append 'Z' to the end of the string to treat it as UTC
        entry.mLastReleased = QDateTime::fromString(mod["lastreleased"_L1].toString() + u'Z', Qt::ISODate);
        for (const auto &id : mod["modidstrs"_L1].toArray()) {
            if (id.isString()) {
                entry.mModIdStrs.append(id.toString());
            }
        }
        for (const auto &tag : mod["tags"_L1].toArray()) {
            if (tag.isString()) {
                entry.mTags.append(tag.toString());
            }
        }
        entries.append(std::move(entry));
    }
    // every entry lacked a modid, the same as an empty list
    if (entries.isEmpty()) {
        qCWarning(cModInfoCache, "Mod list reply has no usable mods, keeping the stored list");
        return std::nullopt;
    }
    return entries;
}

// owns every QSqlDatabase and QSqlQuery of the writer, all are gone when it returns
bool writeModList(QLatin1StringView connection, const QString &dbPath, const QList<ModListEntry> &entries) {
    QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, connection);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qCWarning(cModInfoCache, "Cannot open %s for the mod list: %s", qUtf8Printable(dbPath),
                  qUtf8Printable(db.lastError().text()));
        return false;
    }

    QSqlQuery query{db};
    const auto fail = [&db](const QSqlQuery &failed) {
        qCWarning(cModInfoCache, "Cannot store the mod list: %s", qUtf8Printable(failed.lastError().text()));
        db.rollback();
        return false;
    };

    if (!query.exec(u"PRAGMA foreign_keys = ON"_s) || !db.transaction()) {
        return fail(query);
    }
    for (const QString &table : {u"mod_list_tags"_s, u"mod_list_ids"_s, u"mod_list"_s}) {
        if (!query.exec(u"DELETE FROM %1"_s.arg(table))) {
            return fail(query);
        }
    }

    QSqlQuery insertMod{db};
    insertMod.prepare(u"INSERT INTO mod_list (modid, assetid, name, summary, author, urlalias, side, type, logo,"
                      " downloads, follows, comments, trendingpoints, lastreleased)"
                      " VALUES (:modid, :assetid, :name, :summary, :author, :urlalias, :side, :type, :logo,"
                      " :downloads, :follows, :comments, :trendingpoints, :lastreleased)"_s);

    QSqlQuery insertId{db};
    insertId.prepare(u"INSERT OR IGNORE INTO mod_list_ids (mod_id, modid) VALUES (:mod_id, :modid)"_s);

    QSqlQuery insertTag{db};
    insertTag.prepare(u"INSERT INTO mod_list_tags (modid, position, tag) VALUES (:modid, :position, :tag)"_s);

    for (const ModListEntry &entry : entries) {
        insertMod.bindValue(u":modid"_s, entry.mModId);
        insertMod.bindValue(u":assetid"_s, entry.mAssetId);
        insertMod.bindValue(u":name"_s, entry.mName);
        insertMod.bindValue(u":summary"_s, entry.mSummary);
        insertMod.bindValue(u":author"_s, entry.mAuthor);
        insertMod.bindValue(u":urlalias"_s, entry.mUrlAlias);
        insertMod.bindValue(u":side"_s, entry.mSide);
        insertMod.bindValue(u":type"_s, entry.mType);
        insertMod.bindValue(u":logo"_s, entry.mLogo.toString());
        insertMod.bindValue(u":downloads"_s, entry.mDownloads);
        insertMod.bindValue(u":follows"_s, entry.mFollows);
        insertMod.bindValue(u":comments"_s, entry.mComments);
        insertMod.bindValue(u":trendingpoints"_s, entry.mTrendingPoints);
        insertMod.bindValue(u":lastreleased"_s, epochOrNull(entry.mLastReleased));
        if (!insertMod.exec()) {
            return fail(insertMod);
        }
        for (const QString &id : entry.mModIdStrs) {
            insertId.bindValue(u":mod_id"_s, id);
            insertId.bindValue(u":modid"_s, entry.mModId);
            if (!insertId.exec()) {
                return fail(insertId);
            }
        }
        for (qsizetype position = 0; position < entry.mTags.size(); ++position) {
            insertTag.bindValue(u":modid"_s, entry.mModId);
            insertTag.bindValue(u":position"_s, position);
            insertTag.bindValue(u":tag"_s, entry.mTags.at(position));
            if (!insertTag.exec()) {
                return fail(insertTag);
            }
        }
    }

    if (!db.commit()) {
        qCWarning(cModInfoCache, "Cannot commit the mod list: %s", qUtf8Printable(db.lastError().text()));
        db.rollback();
        return false;
    }
    qCDebug(cModInfoCache, "Stored %lld mods from the mod list", static_cast<long long>(entries.size()));
    return true;
}
} // namespace vsmm::modinfocache
