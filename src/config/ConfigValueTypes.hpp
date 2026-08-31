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

#include <ConfigTypes.hpp>
#include <qqmlintegration.h>

namespace vsmm {
struct GeneralSettingsForeign {
    Q_GADGET
    QML_VALUE_TYPE(generalSettings)
    QML_FOREIGN(vsmm::GeneralSettings)
};

struct PathSettingsForeign {
    Q_GADGET
    QML_VALUE_TYPE(pathSettings)
    QML_FOREIGN(vsmm::PathSettings)
};

struct AppearanceSettingsForeign {
    Q_GADGET
    QML_VALUE_TYPE(appearanceSettings)
    QML_FOREIGN(vsmm::AppearanceSettings)
};

namespace AppearanceForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(vsmm::appearance)
QML_NAMED_ELEMENT(appearance)
} // namespace AppearanceForeign
} // namespace vsmm