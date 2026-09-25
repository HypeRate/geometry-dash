#include "UpdateCheck.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace {
    constexpr auto kLatestReleaseApi = "https://api.github.com/repos/HypeRate/geometry-dash/releases/latest";

    std::optional<std::string> s_pendingUpdate;
    bool s_started = false;

    void onResponse(web::WebResponse const& response) {
        if (!response.ok()) {
            log::info("Update check failed (HTTP {})", response.code());
            return;
        }
        auto json = response.json();
        if (!json) return;

        auto tag = json.unwrap()["tag_name"].asString().unwrapOr("");
        auto latest = VersionInfo::parse(tag);
        if (!latest) {
            log::warn("Update check: can't parse release tag '{}'", tag);
            return;
        }

        auto current = Mod::get()->getVersion();
        if (latest.unwrap() > current) {
            log::info("Update available: {} (installed: {})", tag, current.toVString());
            s_pendingUpdate = tag;
        }
    }
}

namespace update_check {
    void start() {
        if (s_started) return;
        s_started = true;
        if (!Mod::get()->getSettingValue<bool>("check-updates")) return;

        // Only asks GitHub which version is the newest - nothing is sent.
        async::spawn(
            web::WebRequest()
                .header("Accept", "application/vnd.github+json")
                .get(kLatestReleaseApi),
            [](web::WebResponse response) { onResponse(response); }
        );
    }

    std::optional<std::string> takePendingUpdate() {
        auto update = std::move(s_pendingUpdate);
        s_pendingUpdate.reset();
        return update;
    }
}
