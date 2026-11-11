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

#include "ModInfoCacheValues.hpp"
#include <QLoggingCategory>
#include <QTimeZone>

Q_STATIC_LOGGING_CATEGORY(cModInfoCache, "modinfocache");

namespace vsmm::modinfocache {
QVariant epochOrNull(const QDateTime &dateTime) {
    return dateTime.isValid() ? QVariant{dateTime.toSecsSinceEpoch()} : QVariant{};
}

QString versionText(const semver::version<> &version) {
    return QString::fromStdString(version.to_string());
}

QDateTime dateOrInvalid(const QVariant &epoch) {
    return epoch.isNull() ? QDateTime{} : QDateTime::fromSecsSinceEpoch(epoch.toLongLong(), QTimeZone::UTC);
}

std::optional<semver::version<>> parseVersion(const QVariant &text, const QString &modId) {
    const QString version = text.toString();
    auto parsed = semver::try_parse(version.toStdString());
    if (!parsed) {
        qCWarning(cModInfoCache, "%s: Cannot parse stored version '%s'", qUtf8Printable(modId),
                  qUtf8Printable(version));
    }
    return parsed;
}
} // namespace vsmm::modinfocache
