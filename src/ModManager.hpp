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
#include <filesystem>
#include <QNetworkAccessManager>

namespace vsmodchecker {
    class ModManager : public QObject {
        Q_OBJECT

    public:
        struct ModEntry {
            QString name;
            QString version;
            QString author;
            QString modid;
            QString filename;
            QString latestVersion;
            QUrl latestVersionUrl;
        };

        explicit ModManager(QNetworkAccessManager &networkManager, QObject *parent = nullptr);
        void setModsPath(std::filesystem::path modsPath);
        bool initModsList();
        [[nodiscard]] const QList<ModEntry>& getModsList() const;

    private:
        QList<ModEntry> mModsList;
        QNetworkAccessManager &mNetworkManager;
        std::filesystem::path mModsPath;

    signals:
        void modAdded(const ModEntry& mod);

    public slots:
        void checkNewVersions();

    private slots:
        void requestInfoFinished();
    };
} // vsmodchecker
