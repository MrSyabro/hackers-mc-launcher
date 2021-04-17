//
// Created by alex2772 on 4/15/21.
//

#include <Util/VariableHelper.h>
#include "GameProfile.h"
#include "DownloadEntry.h"

DownloadEntry downloadEntryFromJson(const AString& path, const AJsonObject& v)
{
    return DownloadEntry{
            path, v["url"].asString(),
            uint64_t(v["size"].asInt()), false, v["sha1"].asString()
    };
}
/**
 * \brief Converts maven library name notation to path
 *
 *        for example:
 *					optifine:OptiFine:1.14.4_HD_U_F4
 *							 ->
 *        optifine/OptiFine/1.14.4_HD_U_F4/OptiFine-1.14.4_HD_U_F4.jar
 *
 *        Java coders so Java coders.
 *
 * \param name maven notation
 * \return meaningful path
 */
AString javaLibNameToPath(const AString& name)
{
    auto colonSplitted = name.split(':');
    if (colonSplitted.size() == 3)
    {
        colonSplitted[0].replaceAll('.', '/');
        return colonSplitted[0] + '/' + colonSplitted[1] + '/' + colonSplitted[2]
               + '/' + colonSplitted[1] + '-' + colonSplitted[2] + ".jar";
    }
    return "INVALID:" + name;
}

void GameProfile::fromJson(GameProfile& dst, const AString& name, const AJsonObject& json) {
    bool cleanupNeeded = false;

    bool isHackersMcFormat = json.contains("hackers-mc");

    if (isHackersMcFormat)
    {
        // hackers-mc format

        // Download entries
        for (auto& d : json["downloads"].asArray())
        {
            auto o = d.asObject();
            dst.mDownloads << downloadEntryFromJson(o["local"].asString(), o);
            dst.mDownloads.last().mExtract = o["extract"].asBool();
        }

        // game args
        for (auto& d : json["game_args"].asArray())
        {
            GameArg entry;
            entry.mName = d["name"].asString();
            entry.mValue = d["value"].asString();

            for (auto& c : d["conditions"].asArray())
            {
                entry.mConditions << std::pair<AString, AVariant>{c.asArray()[0].asString(), c.asArray()[1].asVariant()};
            }

            dst.mGameArgs << entry;
        }
        // java args
        for (auto& d : json["java_args"].asArray())
        {
            JavaArg entry;
            entry.mName = d["name"].asString();

            for (auto& c : d["conditions"].asArray())
            {
                entry.mConditions << std::pair<AString, AVariant>{c.asArray()[0].asString(), c.asArray()[1].asVariant()};
            }

            dst.mJavaArgs << entry;
        }

        // classpath
        for (auto& e : json["classpath"].asArray())
        {
            dst.mClasspath << e.asString();
        }

        auto settings = json["settings"].asObject();
        dst.mWindowWidth = settings["window_width"].asInt();
        dst.mWindowHeight = settings["window_height"].asInt();
        dst.mIsFullscreen = settings["fullscreen"].asBool();
    }
    else
    {
        // legacy minecraft launcher format

        // Java libraries
        for (auto& v : json["libraries"].asArray())
        {
            if (v.asObject().contains("rules"))
            {
                bool allowed = false;
                for (auto& r : v["rules"].asArray())
                {
                    const auto& rule = r.asObject();
                    bool rulePassed = true;
                    for (auto& kv : rule)
                    {
                        if (kv.first != "action")
                        {
                            if (kv.second.isObject())
                            {
                                auto x = kv.second.asObject();
                                for (auto& var : x)
                                {
                                    if (VariableHelper::getVariableValue(kv.first + '.' + var.first).toString() != var.second
                                            .
                                                    asVariant()
                                            .
                                                    toString()
                                            )
                                    {
                                        rulePassed = false;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                if (VariableHelper::getVariableValue(kv.first).toString() != kv.second
                                        .asVariant().toString())
                                {
                                    rulePassed = false;
                                }
                            }
                            if (!rulePassed)
                                break;
                        }
                    }
                    if (rulePassed)
                        allowed = rule["action"].asString() == "allow";
                }
                if (!allowed)
                    continue;
            }

            AString name = v["name"].asString();

            if (v["downloads"].isObject())
            {
                dst.mDownloads << downloadEntryFromJson("libraries/" + v["downloads"]["artifact"]["path"].asString(),
                                                      v["downloads"]["artifact"].asObject());

                bool extract = v.contains("extract");
                dst.mDownloads.last().mExtract = extract;

                try {
                    auto k = v["downloads"]["classifiers"]["natives-windows"];
                    dst.mDownloads.last().mExtract = false;
                    dst.mDownloads << downloadEntryFromJson("libraries/" + k["path"].asString(), k.asObject());
                    dst.mDownloads.last().mExtract = extract;
                    dst.mClasspath << "libraries/" + k["path"].asString();
                } catch (...) {

                }
            }
            else
            {
                // Minecraft Forge-style entry
                AString url = "https://libraries.minecraft.net/";
                try {
                    url = v["url"].asString();
                    if (!url.endsWith("/"))
                    {
                        url += "/";
                    }
                } catch (...) {}
                dst.mDownloads << DownloadEntry {
                        "libraries/" + javaLibNameToPath(name), url + javaLibNameToPath(name), 0, false, ""
                };
            }

            if (v["downloads"].isObject()) {
                dst.mClasspath << "libraries/" + v["downloads"]["artifact"]["path"].asString();
            }
            else {
                dst.mClasspath << "libraries/" + javaLibNameToPath(name);
            }
        }


        auto args = json["arguments"].asObject();

        unsigned counter = 0;

        auto parseConditions = [](AVector<std::pair<AString, AVariant>>& dst, const AJsonElement& in)
        {
            // find positive conditions

            for (auto& r : in.asArray())
            {
                auto rule = r.asObject();
                if (rule["action"].asString() == "allow")
                {
                    for (auto& r : rule)
                    {
                        if (r.first == "features")
                        {
                            auto features = r.second.asObject();
                            for (auto& feature : features)
                            {
                                dst << std::pair<AString, AVariant>{feature.first, feature.second.asVariant()};
                            }
                        }
                        else if (r.first != "action")
                        {
                            if (r.second.isObject())
                            {
                                auto sub = r.second.asObject();
                                for (auto& subKey : sub)
                                {
                                    dst << std::pair<AString, AVariant>{r.first + '.' + subKey.first, subKey.second.asVariant()};
                                }
                            }
                            else
                            {
                                dst << std::pair<AString, AVariant>{r.first, r.second.asVariant()};
                            }
                        }
                    }
                }
            }
        };

        // Game args
        {
            GameArg arg;

            auto processStringObject = [&](const AString& value)
            {
                if (counter % 2 == 0)
                {
                    arg.mName = value;
                }
                else
                {
                    arg.mValue = value;
                    for (auto it = dst.mGameArgs.begin(); it != dst.mGameArgs.end();)
                    {
                        if (it->mName == arg.mName)
                        {
                            it = dst.mGameArgs.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                    dst.mGameArgs << arg;
                    arg = {};
                }

                counter += 1;
            };

            if (json.contains("minecraftArguments"))
            {
                // old-style args
                AStringVector args = json["minecraftArguments"].asString().split(' ');
                for (auto& s : args)
                    if (!s.empty())
                        processStringObject(s);
            }
            else
            {
                for (const auto& a : args["game"].asArray())
                {
                    if (a.isString())
                    {
                        processStringObject(a.asString());
                    }
                    else if (a.isObject())
                    {
                        // flush last arg
                        if (counter % 2 == 1)
                        {
                            dst.mGameArgs << arg;
                            arg = {};
                            counter += 1;
                        }

                        parseConditions(arg.mConditions, a["rules"]);

                        // Add arguments
                        if (a["value"].isArray())
                        {
                            for (const auto& value : a["value"].asArray())
                            {
                                if (counter % 2 == 0)
                                {
                                    arg.mName = value.asString();
                                }
                                else
                                {
                                    arg.mValue = value.asString();
                                    dst.mGameArgs << arg;
                                    arg.mName.clear();
                                    arg.mValue.clear();
                                }
                                counter += 1;
                            }
                            if (counter % 2 == 1)
                            {
                                dst.mGameArgs << arg;
                            }

                            arg = {};
                        }
                        else if (a["value"].isString())
                        {
                            arg.mName = a["value"].asString();
                            dst.mGameArgs << arg;
                            arg = {};
                        }
                    }
                }
            }
        }

        // JVM args
        for (const auto& a : args["jvm"].asArray())
        {
            if (a.isString())
            {
                dst.mJavaArgs << JavaArg{a.asString()};
            }
            else if (a.isObject())
            {
                AVector<std::pair<AString, AVariant>> conditions;
                parseConditions(conditions, a["rules"]);

                if (a["value"].isString())
                {
                    dst.mJavaArgs << JavaArg{a["value"].asString(), conditions};
                }
                else if (a["value"].isArray())
                {
                    for (auto& i : a["value"].asArray())
                    {
                        dst.mJavaArgs << JavaArg{i.asString(), conditions};
                    }
                }
            }
        }

        if (dst.mJavaArgs.empty())
        {
            // add some important Java args
            dst.mJavaArgs << JavaArg{"-Djava.library.path=${natives_directory}"};
            dst.mJavaArgs << JavaArg{"-cp"};
            dst.mJavaArgs << JavaArg{"${classpath}"};
        }
        cleanupNeeded = true;
    }

    // classpath order fix for Optifine 1.15.2
    if (json.contains("inheritsFrom"))
    {
        /*
        auto tmp1 = dst.mJavaArgs;
        auto tmp2 = dst.mGameArgs;
        launcher->tryLoadProfile(p, json["inheritsFrom"].asString());
        dst.mJavaArgs << tmp1;
        dst.mGameArgs << tmp2;

        // if the profile does not have it's own main jar file, we can copy it from
        // the inherited profile.
        // in theory, it would work recursively too.
        auto mainJarAbsPath = launcher->getSettings()->getGameDir().absoluteFilePath(
                "versions/" + name + "/" + name + ".jar");
        if (!QFile(mainJarAbsPath).exists())
        {
            QFile::copy(
                    launcher->getSettings()->getGameDir().absoluteFilePath("versions/" + dst.mName + "/" + dst.mName + ".jar"),
                    mainJarAbsPath);
        }*/
    }

    if (cleanupNeeded)
        dst.makeClean();

    dst.mName = name;

    if (json["mainClass"].isString())
        dst.mMainClass = json["mainClass"].asString();
    if (json["assets"].isString())
        dst.mAssetsIndex = json["assets"].asString();


    if (!isHackersMcFormat)
    {
        // again, due to Optifine 1.15.2 load order main game jar should be in the end of classpath load order.

        // client jar
        {
            auto path = "versions/" + json["id"].asString() + '/' + json["id"].asString() + ".jar";
            dst.mDownloads << downloadEntryFromJson(path, json["downloads"].asObject()["client"].asObject());
            dst.mClasspath << path;
        }

        // Asset index
        dst.mDownloads << DownloadEntry {
                "assets/indexes/" + dst.mAssetsIndex + ".json",
                json["assetIndex"].asObject()["url"].asString(),
                uint64_t(json["assetIndex"].asObject()["size"].asInt()),
                false,
                json["assetIndex"].asObject()["sha1"].asString()
        };
    }
}

void GameProfile::makeClean() {

}