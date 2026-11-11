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
#include <QString>
#include <QVariant>
#include <semver.hpp>

#include <optional>

namespace vsmm::modinfocache {
[[nodiscard]] QVariant epochOrNull(const QDateTime &dateTime);
[[nodiscard]] QDateTime dateOrInvalid(const QVariant &epoch);
[[nodiscard]] QString versionText(const semver::version<> &version);
[[nodiscard]] std::optional<semver::version<>> parseVersion(const QVariant &text, const QString &modId);
} // namespace vsmm::modinfocache
