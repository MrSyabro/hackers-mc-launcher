//
// Created by alex2772 on 4/20/21.
//

#include <AUI/Logging/ALogger.h>
#include <Model/Settings.h>
#include <AUI/Crypt/AHash.h>
#include <AUI/Traits/iterators.h>
#include <AUI/Platform/AWindow.h>
#include <AUI/IO/FileInputStream.h>
#include <AUI/Curl/ACurl.h>
#include <AUI/Platform/AProcess.h>
#include <Util/VariableHelper.h>
#include "Launcher.h"


void Launcher::play(const User& user, const GameProfile& profile, bool doUpdate) {
    try {
        emit updateStatus("Scanning files to download");

        ALogger::info("== PLAY BUTTON PRESSED ==");
        ALogger::info("User: " + user.username);
        ALogger::info("Profile: " + profile.getName());

        const APath gameFolder = Settings::inst().game_folder;
        ALogger::info("Install path: " + gameFolder);

        const APath assetsJson = gameFolder["assets"]["indexes"][profile.getAssetsIndex() + ".json"];
        ALogger::info("Assets: " + assetsJson);

        auto checkAndDownload = [&] {
            // we should have asset indexes on disk in order to determine how much data will we download
            if (!assetsJson.isRegularFileExists()) {
                // if it does not exist we should find it in downloads array
                AString filepath = "assets/indexes/" + profile.getAssetsIndex() + ".json";

                // the asset is located near the end so using reverse iterator wrap
                for (const auto& download : aui::reverse_iterator_wrap(profile.getDownloads())) {
                    if (download.mLocalPath == filepath) {
                        // here we go. download it
                        auto localFile = gameFolder.file(filepath);
                        localFile.parent().makeDirs();
                        _new<ACurl>(download.mUrl) >> _new<FileOutputStream>(localFile);
                        break;
                    }
                }
            }
            if (!assetsJson.isRegularFileExists()) {
                throw AException("Could not download asset indexes file. Try to reset game profile");
            }

            struct ToDownload {
                AString localPath;
                AString url;
            };

            AVector<ToDownload> toDownload;
            size_t downloadedBytes = 0;
            size_t totalDownloadBytes = 0;

            // regular downloads
            for (auto& download : profile.getDownloads()) {
                auto localFilePath = gameFolder.file(download.mLocalPath);
                if (!localFilePath.isRegularFileExists()) {
                    ALogger::info("[x] To download: " + download.mLocalPath);
                } else if (doUpdate && (download.mHash.empty() ||
                                        AHash::sha1(_new<FileInputStream>(localFilePath)).toHexString() !=
                                        download.mHash)) {
                    ALogger::info("[#] To download: " + download.mLocalPath);
                } else {
                    continue;
                }
                totalDownloadBytes += download.mSize;
                toDownload << ToDownload{download.mLocalPath, download.mUrl};
            }

            // asset downloads
            auto assets = AJson::read(_new<FileInputStream>(assetsJson));

            auto objects = assets["objects"].asObject();

            auto objectsDir = gameFolder["assets"]["objects"];

            for (auto& object : objects) {
                auto hash = object.second["hash"].asString();
                auto path = AString() + hash[0] + hash[1] + '/' + hash;
                auto local = objectsDir[path];

                if (local.isRegularFileExists()) {
                    if (!doUpdate
                        || AHash::sha1(_new<FileInputStream>(local)).toHexString() == hash // check file's sha1
                            ) {
                        continue;
                    }
                }

                ALogger::info("[A] To download: " + object.first);

                totalDownloadBytes += object.second["size"].asInt();
                toDownload << ToDownload{
                        "assets/objects/" + path, "http://resources.download.minecraft.net/" + path
                };
            }

            emit updateTotalDownloadSize(totalDownloadBytes);
            emit updateStatus("Downloading...");

            ALogger::info("== BEGIN DOWNLOAD ==");

            for (auto& d : toDownload) {
                auto local = gameFolder[d.localPath];
                ALogger::info("Downloading: " + d.url + " > " + local);
                emit updateTargetFile(d.localPath);
                local.parent().makeDirs();
                auto url = _new<ACurl>(d.url);
                auto fos = _new<FileOutputStream>(local);

                char buf[0x8000];
                for (int r; (r = url->read(buf, sizeof(buf))) > 0;) {
                    fos->write(buf, r);
                    downloadedBytes += r;
                    if (AWindow::isRedrawWillBeEfficient()) {
                        emit updateDownloadedSize(downloadedBytes);
                    }
                }
            }
        };

        checkAndDownload();

        emit updateTargetFile("");
        emit updateStatus("Checking...");

        checkAndDownload();


        emit updateTargetFile("");
        emit updateStatus("Preparing to launch...");

        VariableHelper::Context c = {
                &user,
                &profile
        };

        // java args
        AStringVector args;
        for (auto& arg : profile.getJavaArgs()) {
            // TODO conditions
            if (VariableHelper::checkConditions(c, arg.mConditions)) {
                args << VariableHelper::parseVariables(c, arg.mName);
            }
        }

        args << profile.getMainClass();

        // game args
        for (auto& arg : profile.getGameArgs()) {
            // TODO conditions
            if (VariableHelper::checkConditions(c, arg.mConditions)) {
                args << VariableHelper::parseVariables(c, arg.mName);
                if (!arg.mValue.empty()) {
                    args << VariableHelper::parseVariables(c, arg.mValue);
                }
            }
        }

        auto java = APath::locate("java");
        ALogger::info(java + " " + args.join(' '));
        int status = AProcess::execute(java, args.join(' '), Settings::inst().game_folder);
        ALogger::info("Child process exit with status "_as + AString::number(status));

    } catch (const AException& e) {
        ALogger::warn("Error running game: " + e.getMessage());
        emit errorOccurred(e.getMessage());
    }
}

void Launcher::download(const DownloadEntry& e) {
}
