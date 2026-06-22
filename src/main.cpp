#include <filesystem>
#include <optional>
#include <fstream>

#include <cpr/cpr.h>

#include "ZipArchive.hpp"
#include "Logger.hpp"
#include <QString>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>

#include <semver/semver.hpp>

static QJsonObject jsonModsRoot;
static vsmodchecker::Logger gLogger("VSModChecker", vsmodchecker::Logger::Level::Info);

namespace {
    constexpr auto GetModUrl(std::string_view modId) {
        return cpr::Url{std::format("https://mods.vintagestory.at/api/mod/{}", modId)};
    }

    std::optional<std::string> GetEnv(std::string_view name) {
        char *value = std::getenv(name.data());
        if (!value) {
            return std::nullopt;
        }
        return std::string(value);
    }

    std::filesystem::path GetConfigPath() {
        if (std::optional<std::string> configPath = GetEnv("XDG_CONFIG_HOME")) {
            return *configPath;
        }
        if (std::optional<std::string> homePath = GetEnv("HOME")) {
            return *homePath + "/.config";
        }

        throw std::runtime_error("Could not find XDG_CONFIG_HOME or HOME environment variables");
    }

    void InitModList(const std::filesystem::path& modPath) {
        for (const auto& entry : std::filesystem::directory_iterator(modPath)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".zip") {
                continue;
            }

            vsmodchecker::ZipArchive zipArchive(entry.path());
            vsmodchecker::ZipArchive::FileIndex zipFileId = zipArchive.getFileIndex("modinfo.json");

            if (zipFileId == -1) {
                gLogger.Error("Failed to locate modinfo.json in zip file: {}", entry.path());
                continue;
            }

            const auto& [fileBuffer, fileSize] = zipArchive.getFileContent(zipFileId);
            QString modVersion, modId;
            try {
                QJsonParseError errorCode{.error = QJsonParseError::NoError};
                auto json = QJsonDocument::fromJson(QByteArray{fileBuffer.get(), fileSize}, &errorCode);
                if (errorCode.error != QJsonParseError::NoError) {
                    gLogger.Error("Failed to parse modinfo.json from zip file: {} Reason: {}", entry.path(), errorCode.errorString().toStdString());
                    continue;
                }
                for (const auto& [key, value] : json.object().asKeyValueRange()) {
                    auto lowercaseKey = key.toString().toLower();

                    if (lowercaseKey == "version") {
                        modVersion = value.toString();
                    } else if (lowercaseKey == "modid") {
                        modId = value.toString();
                    }
                }
            } catch (const std::exception& e) {
                gLogger.Error("Failed to parse modinfo.json from zip file: {} Reason: {}", entry.path(), e.what());
                continue;
            }

            auto modObj = QJsonObject{
                {
                    {"version", modVersion},
                    {"file", QString::fromStdString(entry.path().filename().string())}
                }
            };

            jsonModsRoot[modId] = std::move(modObj);
        }
    }

    /*void qtMsgHandler(QtMsgType type,
                      const QMessageLogContext& ctx,
                      const QString& msg) {
        const std::string s = msg.toStdString();
        switch (type) {
            case QtDebugMsg:    gLogger.Debug("[Qt] {}", s); break;
            case QtInfoMsg:     gLogger.Info ("[Qt] {}", s); break;
            case QtWarningMsg:  gLogger.Warn ("[Qt] {}", s); break;
            case QtCriticalMsg:
            case QtFatalMsg:    gLogger.Error("[Qt] {} ({}:{})",
                                              s,
                                              ctx.file ? ctx.file : "?",
                                              ctx.line);
                break;
        }
    }*/
}

int main(int argc, char* argv[]) {
    std::filesystem::path modPath = GetConfigPath() / "VintagestoryData" / "Mods";
    if (!std::filesystem::exists(modPath)) {
        gLogger.Error("Mods directory does not exist: {}", modPath);
        return 1;
    }

    InitModList(modPath);

    std::vector<std::future<void>> futures;
    for (auto [key, value] : jsonModsRoot.asKeyValueRange()) {
        auto obj = value.toObject();
        futures.emplace_back(std::async(std::launch::async, [modPath, key, version = semver::version::parse(obj["version"].toString().toStdString()), fileName = obj["file"].toString()] {
            cpr::Response response = cpr::Get(GetModUrl(std::string_view{static_cast<const char *>(key.data()), key.size()}));

            if (response.status_code != 200) {
                gLogger.Error("Failed to retrieve mod info for {}: {}", key.toString().toStdString(), response.status_code);
                return;
            }

            if (response.header["content-type"] != "application/json") {
                gLogger.Error("Invalid response format for {}: {}", key.toString().toStdString(), response.header["Content-Type"]);
                return;
            }

            auto doc = QJsonDocument::fromJson(QByteArray(response.text.data(), response.downloaded_bytes));
            auto responseJson = doc.object()["mod"].toObject();
            auto modName = responseJson["name"].toString();
            auto lastestReleaseJsonObj = responseJson["releases"].toArray().first();

            QJsonObject modEntry;
            modEntry["modName"] = modName;
            modEntry["latestVersion"] = lastestReleaseJsonObj["modversion"].toString();
            modEntry["author"] = responseJson["author"];

            gLogger.Debug("Retrieved mod info for {}", modName.toStdString());
            auto latestReleaseVersion = semver::version::parse( lastestReleaseJsonObj["modversion"].toString().toStdString());

            if (latestReleaseVersion > version) {
                /*gLogger.Info("New version available for {}: {}", key, latestReleaseVersion);

                cpr::Url downloadUrl{lastestReleaseJsonObj["mainfile"].get<std::string>()};
                cpr::Response downloadResponse = cpr::Get(downloadUrl);

                if (downloadResponse.status_code != 200) {
                    gLogger.Error("Failed to download mod for {}: {}", modName, downloadResponse.status_code);
                    return;
                }

                if (downloadResponse.header["content-type"] != "application/zip") {
                    gLogger.Error("Invalid response format for {}: {}", modName, downloadResponse.header["content-type"]);
                    return;
                }

                std::error_code ec;
                std::filesystem::remove(modPath / fileName, ec);
                if (ec) {
                    gLogger.Error("Failed to remove old mod file for {}: {}", modName, ec.message());
                    return;
                }

                std::ofstream newFile(modPath / lastestReleaseJsonObj["filename"], std::ios::binary);
                if (!newFile) {
                    gLogger.Error("Failed to create new mod file for {}", modName);
                    return;
                }

                newFile.write(downloadResponse.text.data(), downloadResponse.downloaded_bytes);
                newFile.close();*/
            } else {
                modEntry["latestVersion"] = "latest";
                gLogger.Info("No new version available for {}", modName.toStdString());
            }

            jsonModsRoot[key.toString()] = modEntry;
        }));
    }

    for (const auto& future : futures) {
        future.wait();
    }

    //qInstallMessageHandler(qtMsgHandler);
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("main", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QObject *rootObj = engine.rootObjects().first();
    auto listModel = rootObj->findChild<QObject*>("modModel");

    for (const auto& [key, value] : jsonModsRoot.asKeyValueRange()) {
        auto obj = value.toObject();
        QVariantMap entry;
        entry["modName"] = obj["modName"].toString();
        entry["author"] = obj["author"].toString();
        entry["version"] = obj["version"].toString();
        entry["updateVersion"] = obj["latestVersion"].toString();
        entry["modEnabled"] = true;
        entry["tag"]= "Farming";

        QMetaObject::invokeMethod(listModel, "appendEntry",
                              Q_ARG(QVariant, entry));
    }

    return app.exec();
}
