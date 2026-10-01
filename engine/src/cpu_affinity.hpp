#pragma once

#include <string>

namespace k6wp {

// Matches EngineApp::Log (void(const char* fmt, ...)); may be null.
using AffinityLogFn = void (*)(const char* fmt, ...);

// P3L.2: E-core affinity. mode = config cpu_affinity ("auto" | "all").
// auto + hybrid CPU (distinct EfficiencyClass values) pins the process to the
// min-class (E-core) set; anything else is a logged no-op. Never throws;
// enumeration or mask failures only log.
void ApplyCpuAffinity(const std::string& mode, AffinityLogFn log);

}  // namespace k6wp
