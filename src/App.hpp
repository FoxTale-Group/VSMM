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
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QQmlApplicationEngine>

#include "ModImageProvider.hpp"

namespace vsmodchecker {
class App final : public QGuiApplication {
  public:
    App(int &argc, char *argv[]);
    ~App() override = default;

  private:
    void initQmlEngine(const QString &modsPath);

  private:
    QNetworkAccessManager mNetworkManager;
    QNetworkDiskCache mNetworkDiskCache;
    ModImageProvider *mModImageProvider{nullptr}; // ownership passed to QML engine
    QQmlApplicationEngine mQmlEngine;
};
} // namespace vsmodchecker
