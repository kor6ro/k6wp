#pragma once

namespace k6wp::launcher {

// Process exit codes (also in PrintUsage, and the contract the uninstallers
// read: packaging/installer.nsi + packaging/uninstall.bat). 4 and 5 exist so a
// declined UAC prompt is distinguishable from a broken policy write.
inline constexpr int kExitOk = 0;
inline constexpr int kExitBadFlag = 2;
inline constexpr int kExitSpawnFailure = 3;   // missing sibling / spawn failure
inline constexpr int kExitElevateTimeout = 4;  // elevated worker outlived its wait
inline constexpr int kExitElevateDeclined = 5;  // UAC prompt was dismissed

}  // namespace k6wp::launcher
