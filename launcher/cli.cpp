#include "cli.hpp"

#include <cstdio>
#include <string>

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
                L"  --video <path>       forwarded to engine.exe\n"
                L"  --wallpaper-mode <m> forwarded to engine.exe\n"
                L"  --elevate-lockscreen <on|off>\n"
                L"                       static lockscreen sync: ACLs "
                L"%PROGRAMDATA%\\K6WP, backs up and sets the HKLM "
                L"LockScreenImage policy (UAC prompt, static image only)\n"
                L"  --help, -h           show this help and exit\n"
                L"Exit codes: 0 ok, 2 bad flag, 3 missing sibling / spawn "
                L"failure (also: the elevated lockscreen helper failed), 4 the "
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
    } else if (arg == L"--config" || arg == L"--video" ||
               arg == L"--wallpaper-mode") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr, L"K6WP: error: %ls requires a value\n",
                      arg.c_str());
        return kExitBadFlag;
      }
      // Quote the value; engine ParseCli accepts "--flag value".
      out.engine_extra += L" " + arg + L" \"" + std::wstring(argv[++i]) + L"\"";
    } else if (arg == L"--elevate-lockscreen") {
      if (i + 1 >= argc) {
        std::fwprintf(stderr,
                      L"K6WP: error: --elevate-lockscreen requires on|off\n");
        return kExitBadFlag;
      }
      const std::wstring action = argv[++i];
      if (action != L"on" && action != L"off") {
        std::fwprintf(stderr,
                      L"K6WP: error: --elevate-lockscreen expects on|off, got "
                      L"'%ls'\n",
                      action.c_str());
        return kExitBadFlag;
      }
      out.elevate_lockscreen = true;
      out.elevate_action = action;
      out.want_studio = false;
    } else if (arg == L"--elevated") {
      out.elevated_worker = true;
    } else if (arg == L"--stop") {
      out.stop = true;
      out.want_studio = false;
      out.silent = true;
    } else if (arg.rfind(L"--config=", 0) == 0 ||
               arg.rfind(L"--video=", 0) == 0 ||
               arg.rfind(L"--wallpaper-mode=", 0) == 0) {
      out.engine_extra += L" " + arg;
    } else {
      std::fwprintf(stderr, L"K6WP: error: unknown flag '%ls'\n", arg.c_str());
      return kExitBadFlag;
    }
  }
  return 0;
}

}  // namespace k6wp::launcher
