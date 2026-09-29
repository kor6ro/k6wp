#pragma once

namespace k6wp {

// Public project links (donation touchpoints, About dialog, tray menu).
inline constexpr const char* K6WP_DONATE_URL = "https://ko-fi.com/kor6ro";
inline constexpr const char* K6WP_PROJECT_URL = "https://github.com/kor6ro/k6wp";
inline constexpr const char* K6WP_RELEASES_URL = "https://github.com/kor6ro/k6wp/releases";
// Releases API endpoint for the optional Studio update check (one GET per
// start + manual Help -> "Check for updates").
// Empty or "TODO..." disables the check silently (same as offline); QA can
// point K6WP_UPDATE_URL at a local test server instead.
inline constexpr const char* K6WP_UPDATE_CHECK_URL =
    "https://api.github.com/repos/kor6ro/k6wp/releases/latest";

}  // namespace k6wp
