//
// Created by karol on 6/21/26.
//

#pragma once

#include <filesystem>
#include <zip.h>

namespace vsmodchecker {
    class ZipArchive {
    public:
        class Exception : public std::runtime_error {
        public:
            explicit Exception(const std::string& message) : std::runtime_error(message) {}
            Exception(const std::string& message, int errCode) : std::runtime_error(message), mErrCode(errCode) {}

        private:
            int mErrCode = -1;
        };

        using FileIndex = zip_int64_t;
        using FileContentSize = zip_int64_t;

        explicit ZipArchive(const std::filesystem::path& path);
        ZipArchive(ZipArchive&) = delete("Cannot copy zip archive");
        ZipArchive& operator=(ZipArchive&) = delete("Cannot copy zip archive");

        ZipArchive(ZipArchive&& other) noexcept;
        ZipArchive& operator=(ZipArchive&& other) noexcept;

        [[nodiscard]] FileIndex getFileIndex(std::string_view fileName) const;
        [[nodiscard]] std::pair<std::unique_ptr<char[]>, FileContentSize> getFileContent(FileIndex fileIndex) const;
        ~ZipArchive();

    private:
        zip_t *mZipFile = nullptr;
    };
} // vsmodchecker
