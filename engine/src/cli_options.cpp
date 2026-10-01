// Engine command-line parsing, split out of engine_app.cpp.

#include "cli_options.hpp"

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>

namespace k6wp {
namespace {

void PrintUsage(FILE* out) {
  std::fprintf(out,
      "K6WP engine\n"
      "Usage: engine.exe [options]\n"
      "  --video <path>                 video file to render\n"
      "  --config <path>                config JSON path\n"
      "  --wallpaper-mode <m>           auto | workerw | progman (injection strategy)\n"
      "  --minimized                    tray-only start (autostart marker, no window)\n"
      "  --engine, --silent             aliases of --minimized (compat, keep working)\n"
      "  --help                         show this help and exit\n");
}

}  // namespace

const char* WallpaperModeToString(WallpaperMode mode) {
  switch (mode) {
    case WallpaperMode::kWorkerW:
      return "workerw";
    case WallpaperMode::kProgman:
      return "progman";
    case WallpaperMode::kAuto:
      return "auto";
  }
  return "auto";
}

int ParseCli(int argc, char** argv, CliOptions& out) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      PrintUsage(stdout);
      return 1;
    }
    if (arg.rfind("--", 0) != 0) {
      std::fprintf(stderr, "warning: ignoring unknown argument '%s'\n", arg.c_str());
      continue;
    }
    // Support both "--flag value" and "--flag=value".
    std::string flag = arg;
    std::optional<std::string> inline_value;
    const size_t eq = arg.find('=');
    if (eq != std::string::npos) {
      flag = arg.substr(0, eq);
      inline_value = arg.substr(eq + 1);
    }
    const auto take_value = [&](const char* name) -> std::optional<std::string> {
      if (inline_value) return *inline_value;
      if (i + 1 >= argc) {
        std::fprintf(stderr, "error: %s requires a value\n", name);
        return std::nullopt;
      }
      return std::string(argv[++i]);
    };

    if (flag == "--video") {
      const auto value = take_value("--video");
      if (!value) return 2;
      out.video_path = std::filesystem::u8path(*value).wstring();
    } else if (flag == "--config") {
      const auto value = take_value("--config");
      if (!value) return 2;
      out.config_path = std::filesystem::u8path(*value).wstring();
    } else if (flag == "--wallpaper-mode") {
      const auto value = take_value("--wallpaper-mode");
      if (!value) return 2;
      if (*value == "auto") {
        out.wallpaper_mode = WallpaperMode::kAuto;
      } else if (*value == "workerw") {
        out.wallpaper_mode = WallpaperMode::kWorkerW;
      } else if (*value == "progman") {
        out.wallpaper_mode = WallpaperMode::kProgman;
      } else {
        std::fprintf(stderr, "error: unknown --wallpaper-mode '%s' (expected auto|workerw|progman)\n",
                     value->c_str());
        return 2;
      }
    } else if (flag == "--minimized") {
      if (inline_value) {
        std::fprintf(stderr, "error: --minimized takes no value\n");
        return 2;
      }
      out.minimized = true;
    } else if (flag == "--engine") {
      // --engine is an alias for --minimized (tray-only start, no window)
      out.minimized = true;
    } else if (flag == "--silent") {
      // --silent is an alias for --minimized (tray-only start, no window)
      out.minimized = true;
    } else if (flag == "--restarted") {
      // WER restart marker from RegisterApplicationRestart(L"--restarted", 0).
      // Intentionally ignored: the engine boots normally after a crash.
      if (inline_value) {
        std::fprintf(stderr, "error: --restarted takes no value\n");
        return 2;
      }
    } else if (flag == "--exit-after-ms") {
      const auto value = take_value("--exit-after-ms");
      if (!value) return 2;
      try {
        out.exit_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --exit-after-ms expects an integer, got '%s'\n", value->c_str());
        return 2;
      }
      if (out.exit_after_ms < 0) {
        std::fprintf(stderr, "error: --exit-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-device-lost-after-ms") {
      const auto value = take_value("--simulate-device-lost-after-ms");
      if (!value) return 2;
      try {
        out.simulate_device_lost_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-device-lost-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_device_lost_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-device-lost-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-suspend-after-ms") {
      const auto value = take_value("--simulate-suspend-after-ms");
      if (!value) return 2;
      try {
        out.simulate_suspend_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-suspend-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_suspend_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-suspend-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-dc-after-ms") {
      const auto value = take_value("--simulate-dc-after-ms");
      if (!value) return 2;
      try {
        out.simulate_dc_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-dc-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_dc_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-dc-after-ms must be >= 0\n");
        return 2;
      }
    } else if (flag == "--simulate-monitor-off-after-ms") {
      const auto value = take_value("--simulate-monitor-off-after-ms");
      if (!value) return 2;
      try {
        out.simulate_monitor_off_after_ms = std::stoi(*value);
      } catch (...) {
        std::fprintf(stderr, "error: --simulate-monitor-off-after-ms expects an integer, got '%s'\n",
                     value->c_str());
        return 2;
      }
      if (out.simulate_monitor_off_after_ms < 0) {
        std::fprintf(stderr, "error: --simulate-monitor-off-after-ms must be >= 0\n");
        return 2;
      }
    } else {
      // Unknown --flag: usage to stderr + exit 2 (never ignore silently).
      // Bare positional args (no -- prefix) stay a warning for compat.
      // Hidden developer test flags (--exit-after-ms, --simulate-*-after-ms)
      // keep parsing above; see docs/dev-test-flags.md, never in --help.
      std::fprintf(stderr, "error: unknown flag '%s'\n", arg.c_str());
      PrintUsage(stderr);
      return 2;
    }
  }
  return 0;
}

}  // namespace k6wp
