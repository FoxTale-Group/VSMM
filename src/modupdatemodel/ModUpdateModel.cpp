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

#include "ModUpdateModel.hpp"
#include <QLoggingCategory>
#include <QMetaEnum>

#include <algorithm>

Q_STATIC_LOGGING_CATEGORY(cModUpdateModel, "modupdatemodel");

namespace vsmm {
using namespace Qt::StringLiterals;

ModUpdateModel::ModUpdateModel(QObject *parent) : QAbstractListModel{parent} {}

int ModUpdateModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(mRows.size());
}

QVariant ModUpdateModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= mRows.size()) {
        return {};
    }

    const Row &row = mRows.at(index.row());
    switch (role) {
    case IdRole:
        return row.mId;
    case NameRole:
        return row.mName;
    case FromVersionRole:
        return row.mFromVersion;
    case ToVersionRole:
        return row.mToVersion;
    case StateRole:
        return row.mState;
    case ProgressRole:
        return progress(row);
    case ErrorRole:
        return row.mError;
    default:
        return {};
    }
}

QHash<int, QByteArray> ModUpdateModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(IdRole, "modId"_ba);
    names.insert(NameRole, "modName"_ba);
    names.insert(FromVersionRole, "fromVersion"_ba);
    names.insert(ToVersionRole, "toVersion"_ba);
    names.insert(StateRole, "updateState"_ba);
    names.insert(ProgressRole, "progress"_ba);
    names.insert(ErrorRole, "errorText"_ba);
    return names;
}

void ModUpdateModel::setStore(IModStore *store) {
    if (mStore) {
        qCWarning(cModUpdateModel, "ModStore already set");
        return;
    }
    // GCOVR_EXCL_START
    if (!store) {
        qCFatal(cModUpdateModel, "ModStore is null");
        return;
    }
    // GCOVR_EXCL_STOP

    mStore = store;
    connect(store, &IModStore::modUpdateRequested, this, &ModUpdateModel::onModUpdateRequested);
    connect(store, &IModStore::modUpdateProgress, this, &ModUpdateModel::onModUpdateProgress);
    connect(store, &IModStore::modUpdateInstalling, this, &ModUpdateModel::onModUpdateInstalling);
    connect(store, &IModStore::modUpdateFinished, this, &ModUpdateModel::onModUpdateFinished);
    connect(store, &IModStore::modUpdateFailed, this, &ModUpdateModel::onModUpdateFailed);
    connect(store, &IModStore::modsLoaded, this, &ModUpdateModel::onModsLoaded);
}

int ModUpdateModel::count() const { return static_cast<int>(mRows.size()); }

int ModUpdateModel::finishedCount() const {
    return static_cast<int>(std::ranges::count_if(mRows, [](const Row &row) { return isDone(row.mState); }));
}

int ModUpdateModel::failedCount() const {
    return static_cast<int>(std::ranges::count_if(mRows, [](const Row &row) { return row.mState == Failed; }));
}

double ModUpdateModel::totalProgress() const {
    if (mRows.isEmpty()) {
        return 0.0;
    }
    double sum{0.0};
    for (const auto &row : mRows) {
        sum += isDone(row.mState) ? 1.0 : std::max(progress(row), 0.0);
    }
    return sum / static_cast<double>(mRows.size());
}

bool ModUpdateModel::isActive() const {
    return std::ranges::any_of(mRows, [](const Row &row) { return !isDone(row.mState); });
}

void ModUpdateModel::clear() {
    if (isActive()) {
        qCWarning(cModUpdateModel, "Cannot clear a running update batch");
        return;
    }
    if (mRows.isEmpty()) {
        return;
    }
    beginResetModel();
    mRows.clear();
    mIdToRow.clear();
    endResetModel();
    emit countChanged();
    emit progressChanged();
}

void ModUpdateModel::onModUpdateRequested(QStringList modIds) {
    beginResetModel();
    mRows.clear();
    mIdToRow.clear();
    for (auto &id : modIds) {
        // the entry still holds the installed version, replace happens after the download
        const ModEntry *mod = mStore->find(id);
        if (!mod || mIdToRow.contains(id)) {
            qCDebug(cModUpdateModel, "Skipping %s, unknown or listed twice", qUtf8Printable(id));
            continue;
        }
        mIdToRow.insert(id, static_cast<int>(mRows.size()));
        mRows.append({.mId = std::move(id),
                      .mName = mod->getName().toString(),
                      .mFromVersion = QString::fromStdString(mod->getVersion().to_string()),
                      .mToVersion = QString::fromStdString(mod->getLatestVersion().mVersion.to_string())});
    }
    endResetModel();
    qCDebug(cModUpdateModel, "Update batch of %lld mods", static_cast<long long>(mRows.size()));
    emit countChanged();
    emit progressChanged();
}

void ModUpdateModel::onModUpdateProgress(QStringView modId, qint64 received, qint64 total) {
    const int row = activeRow(modId);
    if (row < 0 || mRows.at(row).mState == Installing) {
        return;
    }

    Row &entry = mRows[row];
    const bool started = entry.mState == Queued;
    entry.mState = Downloading;
    entry.mReceived = received;
    entry.mTotal = total;

    const QModelIndex idx = index(row);
    if (started) {
        qCDebug(cModUpdateModel, "%s: Queued -> Downloading", qUtf8Printable(entry.mId));
        emit dataChanged(idx, idx, {StateRole, ProgressRole});
    } else {
        emit dataChanged(idx, idx, {ProgressRole});
    }
    emit progressChanged();
}

void ModUpdateModel::onModUpdateInstalling(QStringView modId) {
    if (const int row = activeRow(modId); row >= 0) {
        setState(row, Installing);
    }
}

void ModUpdateModel::onModUpdateFinished(QStringView modId) {
    if (const int row = activeRow(modId); row >= 0) {
        setState(row, Done);
        return;
    }

    qCDebug(cModUpdateModel, "Ignoring result for %s, not running in this batch", qUtf8Printable(modId.toString()));
}

void ModUpdateModel::onModUpdateFailed(QStringView modId, QString reason) {
    if (const int row = activeRow(modId); row >= 0) {
        setState(row, Failed, std::move(reason));
        return;
    }

    qCDebug(cModUpdateModel, "Ignoring result for %s, not running in this batch", qUtf8Printable(modId.toString()));
}

void ModUpdateModel::onModsLoaded() {
    for (int row = 0; row < mRows.size(); ++row) {
        if (!isDone(mRows.at(row).mState)) {
            qCWarning(cModUpdateModel, "Update of %s ended without a result", qUtf8Printable(mRows.at(row).mId));
            setState(row, Failed, u"Update ended without a result"_s);
        }
    }
}

double ModUpdateModel::progress(const Row &row) {
    switch (row.mState) {
    case Installing:
    case Done:
        return 1.0;
    case Queued:
        return 0.0;
    case Downloading:
    case Failed:
        break;
    }
    if (row.mTotal > 0) {
        return static_cast<double>(row.mReceived) / static_cast<double>(row.mTotal);
    }
    return row.mState == Downloading ? -1.0 : 0.0;
}

bool ModUpdateModel::isDone(State state) { return state == Done || state == Failed; }

int ModUpdateModel::activeRow(QStringView modId) const {
    const auto it = mIdToRow.constFind(modId);
    if (it == mIdToRow.constEnd() || isDone(mRows.at(*it).mState)) {
        return -1;
    }
    return *it;
}

void ModUpdateModel::setState(int row, State state, QString error) {
    Row &entry = mRows[row];

    const QMetaEnum states = QMetaEnum::fromType<State>();
    qCDebug(cModUpdateModel, "%s: %s -> %s", qUtf8Printable(entry.mId), states.valueToKey(entry.mState),
            states.valueToKey(state));

    entry.mState = state;
    entry.mError = std::move(error);
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {StateRole, ProgressRole, ErrorRole});
    emit progressChanged();
}
} // namespace vsmm
