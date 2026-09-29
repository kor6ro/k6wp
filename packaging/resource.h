// packaging/resource.h - shared numeric resource ids for every app.rc.
//
// Single definition point (Step 6.2d): IDI_APPICON 101 = running/app icon
// (k6wp-on.ico), IDI_APPICON_PAUSED 102 = paused tray icon (k6wp-off.ico).
// Keep in sync with kTrayResourceId / kTrayResourceIdPaused in
// engine/src/tray.hpp (which carries its own constants + this pointer).
// Every app.rc does `#include "resource.h"` and NEVER defines these inline;
// packaging/ is on every target's RC include path via CMake.
#pragma once

#define IDI_APPICON 101
#define IDI_APPICON_PAUSED 102
