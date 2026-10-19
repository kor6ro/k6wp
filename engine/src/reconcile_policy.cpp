#include "reconcile_policy.hpp"

#include <algorithm>
#include <cstdint>
#include <map>

namespace k6wp {

namespace {

// UTF-16 -> UTF-8, self-contained so this module stays windows.h-free (the
// whole point: the decision core is unit-tested headless, with no Win32
// dependency and no link closure beyond the CRT).
void AppendUtf8(std::string& out, std::uint32_t cp) {
  if (cp < 0x80) {
    out.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp < 0x10000) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

std::string NarrowDevice(const std::wstring& wide) {
  std::string out;
  out.reserve(wide.size());
  for (std::size_t i = 0; i < wide.size(); ++i) {
    std::uint32_t cp = static_cast<std::uint32_t>(wide[i]);
    if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < wide.size()) {
      const std::uint32_t low = static_cast<std::uint32_t>(wide[i + 1]);
      if (low >= 0xDC00 && low <= 0xDFFF) {
        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
        ++i;
      }
    }
    AppendUtf8(out, cp);
  }
  return out;
}

std::string DeviceKeyUtf8(const MonitorInfo& mi) {
  if (!mi.device_name.empty()) return NarrowDevice(mi.device_name);
  return std::string();
}

}  // namespace

ReconcileAction DecideReconcileAction(const std::string& trigger) {
  if (trigger == "display-change" || trigger == "monitor-on" ||
      trigger == "device-lost") {
    return ReconcileAction::kConvergeSlots;
  }
  if (trigger == "resume" || trigger == "session-unlock" ||
      trigger == "taskbar-created" || trigger == "setting-change") {
    return ReconcileAction::kReanchorSlots;
  }
  return ReconcileAction::kNone;
}

const char* ReconcileActionName(ReconcileAction action) {
  switch (action) {
    case ReconcileAction::kConvergeSlots:
      return "converge";
    case ReconcileAction::kReanchorSlots:
      return "reanchor";
    case ReconcileAction::kNone:
    default:
      return "none";
  }
}

bool ReconcileActionNeedsStableTopology(ReconcileAction action) {
  // Only the desktop/power actions wait for a settled enumeration. A
  // display change is itself the topology change - waiting for it would
  // only delay the convergence the user is waiting for.
  return action == ReconcileAction::kReanchorSlots;
}

std::string SlotIdentityKey(const MonitorInfo& mi) {
  if (!mi.stable_key.empty()) return mi.stable_key;
  const std::string device = DeviceKeyUtf8(mi);
  if (!device.empty()) return "gdi:" + device;
  return "pos:" + std::to_string(mi.id);
}

bool TopologyStable(const std::vector<MonitorInfo>& first,
                    const std::vector<MonitorInfo>& second) {
  if (first.size() != second.size()) return false;
  // Index the second enumeration by identity so the comparison is
  // order-insensitive. A duplicated identity (ambiguous EDID, two monitors
  // without a device name) makes the topology undecidable -> not stable.
  std::map<std::string, const MonitorInfo*> by_identity;
  for (const MonitorInfo& mi : second) {
    const std::string key = SlotIdentityKey(mi);
    if (!by_identity.emplace(key, &mi).second) return false;
  }
  for (const MonitorInfo& mi : first) {
    const std::string key = SlotIdentityKey(mi);
    const auto it = by_identity.find(key);
    if (it == by_identity.end()) return false;
    const MonitorInfo& other = *it->second;
    if (mi.x != other.x || mi.y != other.y || mi.width != other.width ||
        mi.height != other.height || mi.is_primary != other.is_primary) {
      return false;
    }
  }
  return true;
}

std::string TopologySignature(const std::vector<MonitorInfo>& monitors) {
  std::vector<std::string> parts;
  parts.reserve(monitors.size());
  for (const MonitorInfo& mi : monitors) {
    std::string part = SlotIdentityKey(mi);
    part += '@';
    part += std::to_string(mi.x);
    part += ',';
    part += std::to_string(mi.y);
    part += ',';
    part += std::to_string(mi.width);
    part += 'x';
    part += std::to_string(mi.height);
    part += mi.is_primary ? "p" : "s";
    parts.push_back(std::move(part));
  }
  // The enumeration is sorted (primary first, then device name), but sort
  // anyway so the signature does not depend on the caller's ordering.
  std::sort(parts.begin(), parts.end());
  std::string out;
  for (const std::string& part : parts) {
    if (!out.empty()) out += ';';
    out += part;
  }
  return out;
}

}  // namespace k6wp
