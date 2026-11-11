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

#include <QLatin1StringView>

#include <array>

namespace vsmm::modinfocache {
using namespace Qt::StringLiterals;

inline constexpr std::array SCHEMA{
    "CREATE TABLE mod_details ("
    " mod_id TEXT NOT NULL PRIMARY KEY COLLATE NOCASE,"
    " modid INTEGER NOT NULL, assetid INTEGER, name TEXT, author TEXT, urlalias TEXT, side TEXT, type TEXT,"
    " text TEXT, logofile TEXT, homepageurl TEXT, sourcecodeurl TEXT, issuetrackerurl TEXT,"
    " wikiurl TEXT, trailervideourl TEXT,"
    " downloads INTEGER, follows INTEGER, comments INTEGER, trendingpoints INTEGER,"
    " created INTEGER, lastmodified INTEGER, lastreleased INTEGER,"
    " fetched_at INTEGER NOT NULL) WITHOUT ROWID"_L1,

    "CREATE TABLE mod_detail_tags ("
    " mod_id TEXT NOT NULL COLLATE NOCASE REFERENCES mod_details ON DELETE CASCADE,"
    " position INTEGER NOT NULL, tag TEXT NOT NULL,"
    " PRIMARY KEY (mod_id, position)) WITHOUT ROWID"_L1,

    "CREATE TABLE mod_screenshots ("
    " mod_id TEXT NOT NULL COLLATE NOCASE REFERENCES mod_details ON DELETE CASCADE,"
    " position INTEGER NOT NULL, fileid INTEGER, mainfile TEXT, filename TEXT,"
    " thumbnailfilename TEXT, created INTEGER,"
    " PRIMARY KEY (mod_id, position)) WITHOUT ROWID"_L1,

    "CREATE TABLE mod_releases ("
    " mod_id TEXT NOT NULL COLLATE NOCASE REFERENCES mod_details ON DELETE CASCADE,"
    " position INTEGER NOT NULL, releaseid INTEGER, modidstr TEXT, modversion TEXT,"
    " filename TEXT, mainfile TEXT, fileid INTEGER, downloads INTEGER, created INTEGER, changelog TEXT,"
    " PRIMARY KEY (mod_id, position)) WITHOUT ROWID"_L1,

    "CREATE TABLE mod_release_versions ("
    " mod_id TEXT NOT NULL COLLATE NOCASE, release_position INTEGER NOT NULL,"
    " position INTEGER NOT NULL, gameversion TEXT NOT NULL,"
    " PRIMARY KEY (mod_id, release_position, position),"
    " FOREIGN KEY (mod_id, release_position) REFERENCES mod_releases ON DELETE CASCADE) WITHOUT ROWID"_L1,

    "CREATE TABLE mod_list ("
    " modid INTEGER NOT NULL PRIMARY KEY, assetid INTEGER, name TEXT, summary TEXT, author TEXT,"
    " urlalias TEXT, side TEXT, type TEXT, logo TEXT,"
    " downloads INTEGER, follows INTEGER, comments INTEGER, trendingpoints INTEGER, lastreleased INTEGER)"_L1,

    "CREATE TABLE mod_list_ids ("
    " mod_id TEXT NOT NULL PRIMARY KEY COLLATE NOCASE, modid INTEGER NOT NULL) WITHOUT ROWID"_L1,

    "CREATE TABLE mod_list_tags ("
    " modid INTEGER NOT NULL, position INTEGER NOT NULL, tag TEXT NOT NULL,"
    " PRIMARY KEY (modid, position)) WITHOUT ROWID"_L1,
};

inline constexpr std::array QUERIES{
    // Details
    "SELECT d.modid, d.assetid, d.name, d.author, d.urlalias, d.side, d.type, d.text, d.logofile,"
    " d.homepageurl, d.sourcecodeurl, d.issuetrackerurl, d.wikiurl, d.trailervideourl,"
    " d.downloads, d.follows, d.comments, d.trendingpoints, d.created, d.lastmodified, d.lastreleased"
    " FROM mod_details d"
    " LEFT JOIN mod_list_ids i ON i.mod_id = d.mod_id"
    " LEFT JOIN mod_list l ON l.modid = i.modid"
    " WHERE d.mod_id = :mod_id AND (l.modid IS NULL OR l.lastreleased = d.lastreleased)"_L1,

    // Tags
    "SELECT tag FROM mod_detail_tags WHERE mod_id = :mod_id ORDER BY position"_L1,

    // Screenshots
    "SELECT fileid, mainfile, filename, thumbnailfilename, created"
    " FROM mod_screenshots WHERE mod_id = :mod_id ORDER BY position"_L1,

    // Releases
    "SELECT position, releaseid, modidstr, modversion, filename, mainfile, fileid, downloads, created, changelog"
    " FROM mod_releases WHERE mod_id = :mod_id ORDER BY position"_L1,

    // Release versions
    "SELECT release_position, gameversion"
    " FROM mod_release_versions WHERE mod_id = :mod_id ORDER BY release_position, position"_L1,

    // Delete details
    "DELETE FROM mod_details WHERE mod_id = :mod_id"_L1,

    // Insert details
    "INSERT INTO mod_details (mod_id, modid, assetid, name, author, urlalias, side, type, text, logofile,"
    " homepageurl, sourcecodeurl, issuetrackerurl, wikiurl, trailervideourl,"
    " downloads, follows, comments, trendingpoints, created, lastmodified, lastreleased, fetched_at)"
    " VALUES (:mod_id, :modid, :assetid, :name, :author, :urlalias, :side, :type, :text, :logofile,"
    " :homepageurl, :sourcecodeurl, :issuetrackerurl, :wikiurl, :trailervideourl,"
    " :downloads, :follows, :comments, :trendingpoints, :created, :lastmodified, :lastreleased, :fetched_at)"_L1,

    // Insert tags
    "INSERT INTO mod_detail_tags (mod_id, position, tag) VALUES (:mod_id, :position, :tag)"_L1,

    // Insert screenshot
    "INSERT INTO mod_screenshots (mod_id, position, fileid, mainfile, filename, thumbnailfilename, created)"
    " VALUES (:mod_id, :position, :fileid, :mainfile, :filename, :thumbnailfilename, :created)"_L1,

    // Insert release
    "INSERT INTO mod_releases (mod_id, position, releaseid, modidstr, modversion, filename, mainfile,"
    " fileid, downloads, created, changelog)"
    " VALUES (:mod_id, :position, :releaseid, :modidstr, :modversion, :filename, :mainfile,"
    " :fileid, :downloads, :created, :changelog)"_L1,

    // Insert release version
    "INSERT INTO mod_release_versions (mod_id, release_position, position, gameversion)"
    " VALUES (:mod_id, :release_position, :position, :gameversion)"_L1};
} // namespace vsmm::modinfocache
