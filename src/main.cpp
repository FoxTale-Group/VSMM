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

#include "App.hpp"

template <> struct std::formatter<QString, char> : std::formatter<std::string_view, char> {
    auto format(const QString &s, std::format_context &ctx) const {
        // Transcode UTF-16 -> UTF-8 once, then defer to string_view formatter.
        const QByteArray utf8 = s.toUtf8();
        return std::formatter<std::string_view, char>::format(
            std::string_view{utf8.constData(), static_cast<std::size_t>(utf8.size())}, ctx);
    }
};

namespace {
QtMessageHandler qtMsgHandlerOld;
void qtMsgHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    static constexpr std::string_view COLOR_GREEN{"\033[1;32m"};
    static constexpr std::string_view COLOR_RED{"\033[1;31m"};
    static constexpr std::string_view COLOR_YELLOW{"\033[1;33m"};
    static constexpr std::string_view COLOR_WHITE{"\033[0;37m"};
    static constexpr std::string_view COLOR_RESET{"\033[0m"};
    std::string res;
    switch (type) {
#ifdef DEBUG
    case QtDebugMsg:
        res = std::format("{}{}{}", COLOR_WHITE, msg, COLOR_RESET);
        break;
#endif
    case QtInfoMsg:
        res = std::format("{}{}{}", COLOR_GREEN, msg, COLOR_RESET);
        break;
    case QtWarningMsg:
        res = std::format("{}{}{}", COLOR_YELLOW, msg, COLOR_RESET);
        break;

    case QtCriticalMsg:
        res = std::format("{}{}{}", COLOR_RED, msg, COLOR_RESET);
        break;
    case QtFatalMsg:
        res = std::format("{}{} ({}:{}){}", COLOR_RED, msg, ctx.file ? ctx.file : "?", ctx.line, COLOR_RESET);
        break;
    default:
        break;
    }

    if (qtMsgHandlerOld && !res.empty()) {
        qtMsgHandlerOld(type, ctx, QString::fromStdString(res));
    }
}
} // namespace

int main(int argc, char *argv[]) {
    qSetMessagePattern("[%{time hh:mm:ss.zzz}] %{type} %{if-debug}%{file}:%{line} %{endif}- %{message}");
    qtMsgHandlerOld = qInstallMessageHandler(qtMsgHandler);

#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
#endif

    vsmm::App app(argc, argv);

    return vsmm::App::exec();
}
