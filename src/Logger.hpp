//
// Created by karol on 2/14/26.
//

#pragma once
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <iterator>
#include <mutex>
#include <string>

namespace vsmodchecker {

class Logger {
public:
    enum class Level : std::uint8_t {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };
    explicit Logger(const std::string_view tagName)
        : mTagName { tagName }
    {
    }
    Logger(const std::string_view tagName, const Level level)
        : mTagName { tagName }
        , mLevel { level }
    {
    }

    template <typename... Args>
    void Debug(std::string_view text, Args&&... args) const
    {
#ifndef NDEBUG
        PrintLog<Level::Debug>(text, std::forward<Args>(args)...);
#endif
    }

    template <typename... Args>
    void Info(std::string_view text, Args&&... args) const
    {
        PrintLog<Level::Info>(text, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Warn(std::string_view text, Args&&... args) const
    {
        PrintLog<Level::Warning>(text, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Error(std::string_view text, Args&&... args) const
    {
        PrintLog<Level::Error>(text, std::forward<Args>(args)...);
    }

private:
    template <Level level, typename... Args>
    void PrintLog(const std::string_view text, Args&&... args) const
    {
        if (mLevel > level) {
            return;
        }

        constexpr auto getStream = [] -> std::ostream& {
            if (level >= Level::Error) {
                return std::cerr;
            }

            return std::cout;
        };

        constexpr auto getLevelTag = [] -> std::string_view {
            switch (level) {
            case Level::Debug:
                return "DEBUG";
            case Level::Info:
                return "INFO";
            case Level::Warning:
                return "WARN";
            default:
                return "ERR";
            }
        };

        constexpr auto getColor = [] {
            switch (level) {
            case Level::Debug:
                return COLOR_WHITE;
            case Level::Info:
                return COLOR_GREEN;
            case Level::Warning:
                return COLOR_YELLOW;
            case Level::Error:
                return COLOR_RED;
            default:
                return COLOR_RESET;
            }
        };

        std::scoped_lock scopedLock { mPrintMutex };
        auto& stream = getStream();
        std::format_to(std::ostream_iterator<char>(stream), "{}{} [{}] [{}] {}{}\n", getColor(), getTime(), mTagName, getLevelTag(), std::vformat(text, std::make_format_args(args...)), COLOR_RESET);
        stream.flush();
    }

    static std::string getTime()
    {
        const auto time = std::chrono::system_clock::now();
        return std::vformat("{:%FT%T}", std::make_format_args(time));
    }

    static constexpr std::string_view COLOR_GREEN { "\033[1;32m" };
    static constexpr std::string_view COLOR_RED { "\033[1;31m" };
    static constexpr std::string_view COLOR_YELLOW { "\033[1;33m" };
    static constexpr std::string_view COLOR_WHITE { "\033[0;37m" };
    static constexpr std::string_view COLOR_RESET { "\033[0m" };

    mutable std::mutex mPrintMutex;
    std::string mTagName;
    Level mLevel { Level::Debug };
};

}
