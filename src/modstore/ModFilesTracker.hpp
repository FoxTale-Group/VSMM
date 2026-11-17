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

#include <QDateTime>
#include <QDir>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <chrono>
#include <utility>

namespace vsmm::modstore {
class ModFilesTracker final : public QObject {
    Q_OBJECT

  public:
    explicit ModFilesTracker(QObject *parent = nullptr);

    void reset(const QList<QDir> &modsDirs);
    void refresh(const QString &path, const QString &modId = {});
    void setModId(const QString &path, const QString &modId);
    [[nodiscard]] QStringList findDuplicates(const QString &modId, const QString &exceptPath) const;

  signals:
    void filesChanged(const QStringList &found, const QStringList &removed);

  private:
    static constexpr std::chrono::seconds DIR_CHANGE_DELAY{1};

    void onDirsChanged();

    struct KnownZip {
        explicit KnownZip(QDateTime modified, QString modId = {})
            : mModified{std::move(modified)}, mModId{std::move(modId)} {}
        QDateTime mModified;
        QString mModId;
    };

    QFileSystemWatcher mWatcher;
    // zips per watched dir
    QHash<QString, QHash<QString, KnownZip>> mKnownZips;
    QTimer mDirChangeDelay;
};
} // namespace vsmm::modstore
