#include "cpu_affinity.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <vector>

namespace k6wp {

void ApplyCpuAffinity(const std::string& mode, AffinityLogFn log) {
  // P3L.2: opted out or explicitly "all" — no-op with a log line so the
  // choice is auditable in engine.log.
  DWORD_PTR proc_mask = 0, sys_mask = 0;
  GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask);
  if (mode != "auto") {
    if (log) {
      log("affinity: mode=%s, no change (mask=0x%llx)", mode.c_str(),
          (unsigned long long)proc_mask);
    }
    return;
  }
  // Enumerate physical cores with efficiency classes. Hybrid = at least
  // two distinct EfficiencyClass values. E-cores = the MINIMUM class set:
  // verified empirically on i7-12650H (P=1 SMT, E=0 single-thread); every
  // (class, mask) pair is logged so the choice is auditable on any rig.
  DWORD len = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &len);
  std::vector<char> buf(len > 0 ? len : 1);
  auto* info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
      buf.data());
  if (len == 0 ||
      !GetLogicalProcessorInformationEx(RelationProcessorCore, info, &len)) {
    if (log) {
      log("warning: affinity auto: core enumeration failed (error %lu), "
          "no change (mask=0x%llx)",
          GetLastError(), (unsigned long long)proc_mask);
    }
    return;
  }
  BYTE min_class = 255, max_class = 0;
  bool any = false;
  for (char* p = buf.data(); p < buf.data() + len;) {
    const auto* e =
        reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(p);
    if (e->Relationship == RelationProcessorCore) {
      any = true;
      const BYTE c = e->Processor.EfficiencyClass;
      if (c < min_class) min_class = c;
      if (c > max_class) max_class = c;
    }
    if (e->Size == 0) break;  // corrupt list guard
    p += e->Size;
  }
  if (!any || min_class == max_class) {
    if (log) {
      log("affinity: auto, non-hybrid CPU (single efficiency class), "
          "no change (mask=0x%llx)",
          (unsigned long long)proc_mask);
    }
    return;
  }
  DWORD_PTR e_mask = 0;
  for (char* p = buf.data(); p < buf.data() + len;) {
    const auto* e =
        reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(p);
    if (e->Relationship == RelationProcessorCore) {
      if (log) {
        log("affinity: core class=%u group=%u mask=0x%llx%s",
            (unsigned)e->Processor.EfficiencyClass,
            (unsigned)e->Processor.GroupMask[0].Group,
            (unsigned long long)e->Processor.GroupMask[0].Mask,
            e->Processor.EfficiencyClass == min_class ? " (E)" : "");
      }
      if (e->Processor.EfficiencyClass == min_class) {
        if (e->Processor.GroupMask[0].Group != 0) {
          if (log) {
            log("warning: affinity auto: E-core outside group 0, "
                "no change (multi-group unsupported)");
          }
          return;
        }
        e_mask |= e->Processor.GroupMask[0].Mask;
      }
    }
    if (e->Size == 0) break;
    p += e->Size;
  }
  if (e_mask == 0 || (e_mask & proc_mask) == 0 || e_mask == proc_mask) {
    if (log) {
      log("warning: affinity auto: degenerate E-mask 0x%llx, no change",
          (unsigned long long)e_mask);
    }
    return;
  }
  if (!SetProcessAffinityMask(GetCurrentProcess(), e_mask & proc_mask)) {
    if (log) {
      log("warning: affinity auto: SetProcessAffinityMask(0x%llx) failed "
          "(error %lu)",
          (unsigned long long)(e_mask & proc_mask), GetLastError());
    }
    return;
  }
  if (log) {
    log("affinity: auto, hybrid CPU, pinned to E-cores (mask=0x%llx)",
        (unsigned long long)(e_mask & proc_mask));
  }
}

}  // namespace k6wp
