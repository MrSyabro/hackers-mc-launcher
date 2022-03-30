#pragma once


#include <AUI/Common/AString.h>
#include <AUI/Json/AJson.h>

struct LauncherRule {
    enum class Action {
        ALLOW,
        DISALLOW
    } action;
    AVector<std::pair<AString, AString>> conditions;
};

AJSON_FIELDS(LauncherRule,
             (action, "action")
             (conditions, "conditions"))

template<>
struct AJsonConv<LauncherRule::Action> {
    static AJson toJson(const LauncherRule::Action& rule) {
        switch (rule) {
            case LauncherRule::Action::ALLOW: return "allow";
            case LauncherRule::Action::DISALLOW: return "disallow";
        }
    }
    static LauncherRule::Action fromJson(const AJson& e) {
        if (e.asString() == "disallow") {
            return LauncherRule::Action::DISALLOW;
        }
        return LauncherRule::Action::ALLOW;
    }
};
using Rules = AVector<LauncherRule>;

struct DownloadEntry {
    AString mLocalPath;
    AString mUrl;
    uint64_t mSize = 0;
    bool mExtract = false;
    AString mHash;
    Rules mConditions;
};

