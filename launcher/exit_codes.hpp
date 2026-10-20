#pragma once

// Process exit codes (also in PrintUsage, and the contract the uninstallers
// read: packaging/installer.nsi + packaging/uninstall.bat). 4 and 5 exist so a
// declined UAC prompt is distinguishable from a broken policy write.
//
// The numbers live in shared/lockscreen_exit.hpp: the Studio needs them too
// (it maps the launcher's exit code onto the lock-screen toggle), and there
// must be exactly ONE definition of the contract.
#include "lockscreen_exit.hpp"

namespace k6wp::launcher {

using k6wp::kExitBadFlag;
using k6wp::kExitElevateDeclined;
using k6wp::kExitElevateTimeout;
using k6wp::kExitOk;
using k6wp::kExitSpawnFailure;

}  // namespace k6wp::launcher
