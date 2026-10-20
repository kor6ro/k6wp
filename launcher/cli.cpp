#include "cli.hpp"

#include <cstdio>
#include <string>

#include "still.hpp"  // ParseStillRes: the shared "WxH" contract

namespace k6wp::launcher {

void PrintUsage(FILE* out) {
  // fputws, not fwprintf (which the rest of this file uses): this text has no
  // substitutions, and its literal %PROGRAMDATA% would be read as a conversion
  // specifier, pulling a vararg that does not exist.
  std::fputws(
                L"K6WP launcher (Qt-free dispatcher)\n"
                L"Usage: K6WP.exe [options]\n"
                L"  (no args)            ensure engine, then launch studio\n"
                L"  --studio             same as default\n"
                L"  --engine             ensure engine resident, open nothing\n"
                L"  --engine --silent    autostart path (engine only)\n"
                L"  --silent             ensure engine only (autostart path)\n"
                L"  --minimized --silent alias of --engine --silent\n"
                L"  --minimized          passthrough marker forwarded to engine\n"
                L"  --stop               uninstall helper: stop the K6WP "
                L"processes that live in this K6WP.exe's own folder "
                L"(engine, Studio, launcher) - never by image name. Sends the "
                L"engine an IPC quit first so it can restore the OS "
                L"wallpaper, then escalates per PID.\n"
                L"  --config <path>      forwarded to engine.exe\n"
                L"  --video <path>       forwarded to engine.exe (or the video "
                L"for --apply-still)\n"
                L"  --wallpaper-mode <m> forwarded to engine.exe\n"
                L"  --apply-still        per-user still helper: apply the first "
                L"frame of --video as the desktop wallpaper and/or the "
                L"lock-screen image (no administrator rights, no UAC)\n"
                L"  --apply-still --restore\n"
                L"                       put the saved original wallpaper and "
                L"lock-screen image back\n"
                L"  --apply-still --wallpaper | --lockscreen\n"
                L"                       limit the operation to one side\n"
                L"  --offset-s <n>       frame seek offset (default: the "
                L"studio setting, 0 = first frame)\n"
                L"  --res <WxH>          still resolution (default: the "
                L"primary monitor)\n"
                L"  --elevate-lockscreen off\n"
                L"                       legacy cleanup only: removes the old "
                L"HKLM LockScreenImage policy (UAC prompt)\n"
                L"  --delay-ms <n>       wait n ms (0..600000) before acting "
                L"(used by the engine's hang watchdog)\n"
                L"  --restarted          forwarded to engine.exe so the "
                L"restart counts against the crash-restart backoff\n"
                L"  --help, -h           show this help and exit\n"
                L"Exit codes: 0 ok, 2 bad flag, 3 missing sibling / spawn "
                L"failure (also: the elevated legacy cleanup failed), 4 the "
                L"elevated helper was still running when its wait budget "
                L"expired, 5 the UAC prompt was declined.\n",
                out);
}

int ParseArgs(int argc, wchar_t** argv, Options& out) {
  for (int i = 1; i < argc; ++i) {
    const std::wstring arg = argv[i];
    if (arg == L"--help" || arg == L"-h") {
      out.show_help = true;
      return 1;
    } else if (arg == L"--studio") {
      out.want_studio = true;
    } else if (arg == L"--engine") {
      out.want_studio = false;
    } else if (arg == L"--silent") {
      out.want_studio = false;
      out.silent = true;
    } else if (arg == L"--minimized") {
      out.minimized = true;
      out.engine_extra += L" --minimized";
    } else if (arg == L"--apply-still") {
      out.apply_still = true;
      out.want_studio = false;
      out.silent = true;
    } else if (arg == L"--restore") {
      out.still_restore = true;
    } else if (arg == L"--wallpaper") {
      out.still_wallpaper = true;
    } else if (arg == L"--lockscreen") {
      out.still_lockscreen = true;
    } else if (arg == L"--offset-s") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr, L"K6WP: error: --offset-s requires a value\n");
        return kExitBadFlag;
      }
      const wchar_t* v = argv[++i];
      wchar_t* end = nullptr;
      const double value = wcstod(v, &end);
      if (end == v || value < 0.0 || value > 3600.0) {
        std::fwprintf(stderr,
                      L"K6WP: error: --offset-s expects 0..3600, got %ls\n", v);
        return kExitBadFlag;
      }
      out.still_has_offset = true;
      out.still_offset_sec = value;
    } else if (arg == L"--res") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr, L"K6WP: error: --res requires WxH\n");
        return kExitBadFlag;
      }
      out.still_res = argv[++i];
      int width = 0;
      int height = 0;
      if (!k6wp::ParseStillRes(out.still_res, width, height)) {
        std::fwprintf(stderr,
                      L"K6WP: error: --res expects WxH (e.g. 1920x1080), got "
                      L"'%ls'\n",
                      out.still_res.c_str());
        return kExitBadFlag;
      }
    } else if (arg == L"--config" || arg == L"--video" ||
               arg == L"--wallpaper-mode") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr, L"K6WP: error: %ls requires a value\n",
                      arg.c_str());
        return kExitBadFlag;
      }
      const std::wstring value = argv[++i];
      // --video is BOTH the engine passthrough flag and the still helper's
      // input. The value is captured here; the engine passthrough below still
      // gets it, and RunApplyStill reads opts.still_video (the helper path
      // never reaches EnsureEngine).
      if (arg == L"--video") out.still_video = value;
      // Quote the value; engine ParseCli accepts "--flag value".
      out.engine_extra += L" " + arg + L" \"" + value + L"\"";
    } else if (arg == L"--elevate-lockscreen") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr,
                      L"K6WP: error: --elevate-lockscreen requires off\n");
        return kExitBadFlag;
      }
      const std::wstring action = argv[++i];
      if (action != L"off") {
        // The "on" direction used the HKLM LockScreenImage policy + ACL grant
        // and is gone: the lock screen is per-user now (--apply-still).
        std::fwprintf(stderr,
                      L"K6WP: error: --elevate-lockscreen expects off (the "
                      L"policy-based 'on' mode was removed; use --apply-still "
                      L"for the per-user lock screen)\n");
        return kExitBadFlag;
      }
      out.elevate_lockscreen = true;
      out.elevate_action = action;
      out.want_studio = false;
    } else if (arg == L"--elevated") {
      out.elevated_worker = true;
    } else if (arg == L"--restarted") {
      // Passthrough marker for the engine: the engine counts a
      // "--restarted" launch against its crash-restart backoff, so a
      // watchdog restart cannot become a restart loop.
      out.engine_extra += L" --restarted";
    } else if (arg == L"--delay-ms") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr, L"K6WP: error: --delay-ms requires a value\n");
        return kExitBadFlag;
      }
      const wchar_t* v = argv[++i];
      wchar_t* end = nullptr;
      const long value = wcstol(v, &end, 10);
      if (end == v || value < 0 || value > 600000) {
        std::fwprintf(stderr,
                      L"K6WP: error: --delay-ms expects 0..600000, got %ls\n",
                      v);
        return kExitBadFlag;
      }
      out.delay_ms = static_cast<int>(value);
    } else if (arg == L"--stop") {
      out.stop = true;
      out.want_studio = false;
      out.silent = true;
    } else if (arg.rfind(L"--config=", 0) == 0 ||
               arg.rfind(L"--video=", 0) == 0 ||
               arg.rfind(L"--wallpaper-mode=", 0) == 0) {
      if (arg.rfind(L"--video=", 0) == 0) {
        out.still_video = arg.substr(std::wstring(L"--video=").size());
      }
      out.engine_extra += L" " + arg;
    } else {
      std::fwprintf(stderr, L"K6WP: error: unknown flag '%ls'\n", arg.c_str());
      return kExitBadFlag;
    }
  }

  // --apply-still validation: the selectors/overrides only mean something for
  // the helper, and apply and restore are mutually exclusive.
  if (!out.apply_still &&
      (out.still_restore || out.still_wallpaper || out.still_lockscreen ||
       out.still_has_offset || !out.still_res.empty())) {
    std::fwprintf(stderr,
                  L"K6WP: error: --restore/--wallpaper/--lockscreen/"
                  L"--offset-s/--res require --apply-still\n");
    return kExitBadFlag;
  }
  if (out.apply_still) {
    if (out.still_restore && (out.still_has_offset || !out.still_res.empty())) {
      std::fwprintf(stderr,
                    L"K6WP: error: --restore cannot be combined with "
                    L"--offset-s/--res\n");
      return kExitBadFlag;
    }
    if (!out.still_restore && out.still_video.empty()) {
      std::fwprintf(stderr,
                    L"K6WP: error: --apply-still requires --video <path> (or "
                    L"--restore)\n");
      return kExitBadFlag;
    }
  }
  return 0;
}

}  // namespace k6wp::launcher
