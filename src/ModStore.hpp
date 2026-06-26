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

#include "ModEntry.hpp"

#include <QObject>
#include <QHash>
#include <qqmlintegration.h>

namespace vsmodchecker {
    class ModStore : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(int count READ count NOTIFY modsModified)
        Q_PROPERTY(int updates READ updates NOTIFY modsModified)
        Q_PROPERTY(bool reloading READ reloading NOTIFY modsModified)

    public:
        explicit ModStore(QObject *parent = nullptr);

        const ModEntry& add(ModEntry mod);
        const ModEntry& replace(ModEntry mod);
        Q_INVOKABLE void reload();
        Q_INVOKABLE void load(const QString& filePath);

        [[nodiscard]] bool contains(const QString &id) const;
        [[nodiscard]] const ModEntry* find(const QString &id) const;
        [[nodiscard]] int count() const;
        [[nodiscard]] int updates() const;
        [[nodiscard]] int reloading() const;

    signals:
        void modsModified();
        void modAdded(const ModEntry &mod);
        void modUpdated(const ModEntry &mod);
        void modsReloading();
        void modAddedFromGUI(const QString& filePath);

    public slots:
        void onModsReloaded();

    private:
        template<typename Obj, typename Signal, typename... Args>
        void emitSignal(Signal&& signal, Obj* obj, Args&&... args) {
            emit (obj->*signal)(std::forward<Args>(args)...);
            emit modsModified();
        }

        QHash<QString, ModEntry> mMods;
        bool mModsBeingReloaded{true};
    };
} // vsmodchecker
