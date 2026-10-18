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

#include <IModStore.hpp>
#include <ModUpdateModelExport.hpp>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <qqmlintegration.h>

namespace vsmm {
// one row per mod of the current update batch, rows outlive the store entries they started from
class MODUPDATEMODEL_EXPORT ModUpdateModel : public QAbstractListModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ModUpdateModel)
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int finishedCount READ finishedCount NOTIFY progressChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY progressChanged)
    Q_PROPERTY(double totalProgress READ totalProgress NOTIFY progressChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY progressChanged)

  public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        FromVersionRole,
        ToVersionRole,
        StateRole,
        ProgressRole,
        ErrorRole
    };

    enum State { Queued, Downloading, Installing, Done, Failed };
    Q_ENUM(State)

    explicit ModUpdateModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setStore(IModStore *store);

    [[nodiscard]] int count() const;
    [[nodiscard]] int finishedCount() const;
    [[nodiscard]] int failedCount() const;
    // range between 0 and 1
    [[nodiscard]] double totalProgress() const;
    [[nodiscard]] bool isActive() const;

    // drops a finished batch, does nothing while one is running
    Q_INVOKABLE void clear();

  signals:
    void countChanged();    // NOTIFY count - QML bindings only
    void progressChanged(); // NOTIFY finishedCount/failedCount/totalProgress/active - QML bindings only

  private slots:
    void onModUpdateRequested(QStringList modIds);
    void onModUpdateProgress(QStringView modId, qint64 received, qint64 total);
    void onModUpdateInstalling(QStringView modId);
    void onModUpdateFinished(QStringView modId);
    void onModUpdateFailed(QStringView modId, QString reason);
    void onModsLoaded();

  private:
    struct Row {
        QString mId;
        QString mName;
        QString mFromVersion;
        QString mToVersion;
        State mState{Queued};
        qint64 mReceived{0};
        qint64 mTotal{-1};
        QString mError{};
    };

    // -1 when unknown size
    [[nodiscard]] static double progress(const Row &row);
    [[nodiscard]] static bool isDone(State state);
    // row of a mod still in progress, -1 for unknown or finished ones
    [[nodiscard]] int activeRow(QStringView modId) const;
    void setState(int row, State state, QString error = {});

    IModStore *mStore{nullptr};
    QList<Row> mRows;
    QHash<QString, int> mIdToRow; // mod id -> row index
};
} // namespace vsmm
