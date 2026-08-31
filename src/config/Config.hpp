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

#include "ConfigValueTypes.hpp"
#include <ConfigExport.hpp>
#include <IConfig.hpp>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <qqmlintegration.h>

namespace vsmm {
class CONFIG_EXPORT Config : public IConfig {
    Q_OBJECT
    QML_NAMED_ELEMENT(Config)
    QML_SINGLETON

  public:
    Config();
    ~Config() override;
    void validate() override;

    void setFavorites(QStringList favorites) override;

    void setGeneral(const GeneralSettings &data) override;
    void setPaths(const PathSettings &data) override;
    void setAppearance(const AppearanceSettings &data) override;

    Q_INVOKABLE void saveToFile() const;

  private:
    QFile mConfigFile;
};
} // namespace vsmm