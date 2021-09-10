#pragma once


#include <Model/GameProfile.h>
#include <Model/Account.h>

class Launcher: public AObject {
private:
    void download(const DownloadEntry& e);

public:
    void play(const Account& user, const GameProfile& profile, bool doUpdate = false);

signals:
    emits<AString> updateStatus;
    emits<size_t> updateDownloadedSize;
    emits<size_t> updateTotalDownloadSize;
    emits<AString> updateTargetFile;
    emits<AString> errorOccurred;
};

