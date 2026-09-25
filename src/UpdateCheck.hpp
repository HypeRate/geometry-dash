#pragma once

#include <optional>
#include <string>

// The mod is distributed through GitHub releases, so Geode can't update it by
// itself. Once per game start we ask GitHub for the latest release; if it is
// newer than this version, the main menu offers the download.
namespace update_check {
    // Starts the request (does nothing if disabled in the settings).
    void start();

    // Version tag of a newer release that hasn't been shown yet this session.
    // Returns it only once.
    std::optional<std::string> takePendingUpdate();

    // Page the player is sent to for the download.
    constexpr auto kReleasesPage = "https://github.com/HypeRate/geometry-dash/releases/latest";
}
