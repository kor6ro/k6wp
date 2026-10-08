// engine/src/ipc_marshal.cpp
// Worker-side validators for the PostMessage-marshaled IPC paths (see the
// header for the threading contract).
#include "ipc_marshal.hpp"

#include <cctype>
#include <filesystem>

#include "ipc_protocol.hpp"
#include "thirdparty/json.hpp"

namespace k6wp {
namespace {

bool IsGdiDeviceName(const std::string& d) {
  const std::string prefix = "\\\\.\\DISPLAY";
  if (d.size() <= prefix.size()) return false;
  if (d.compare(0, prefix.size(), prefix) != 0) return false;
  for (size_t i = prefix.size(); i < d.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(d[i]))) return false;
  }
  return true;
}

}  // namespace

std::optional<int> ParseSetMonitorPayload(const std::string& payload_json) {
  int id = -1;
  try {
    const nlohmann::json payload = nlohmann::json::parse(payload_json);
    if (!payload.is_object()) return std::nullopt;
    const nlohmann::json* value = nullptr;
    if (payload.contains("monitor")) {
      value = &payload.at("monitor");
    } else if (payload.contains("monitor_id")) {
      value = &payload.at("monitor_id");
    }
    if (value == nullptr || !value->is_number_integer()) return std::nullopt;
    // Range-check before narrowing: is_number_integer() is also true for
    // number_unsigned, and get<int>() on an out-of-range value is an
    // implementation-defined wrap (e.g. 4294967295 -> -1 = "all screens").
    const long long raw = value->get<long long>();
    if (raw < -1 || raw > 2147483647LL) return std::nullopt;
    id = static_cast<int>(raw);
  } catch (...) {
    return std::nullopt;
  }
  if (id < -1) return std::nullopt;
  return id;
}

std::optional<std::string> ValidateSetVideoPayload(
    const std::string& payload_json) {
  std::string utf8_path;
  try {
    const nlohmann::json payload = nlohmann::json::parse(payload_json);
    if (!payload.is_object() || !payload.contains("path") ||
        !payload.at("path").is_string()) {
      return std::nullopt;
    }
    utf8_path = payload.at("path").get<std::string>();
  } catch (...) {
    return std::nullopt;
  }
  if (utf8_path.empty()) return std::nullopt;
  std::error_code ec;
  const std::filesystem::path fs_path = std::filesystem::u8path(utf8_path);
  if (!std::filesystem::exists(fs_path, ec) ||
      !std::filesystem::is_regular_file(fs_path, ec)) {
    return std::nullopt;
  }
  return utf8_path;
}

std::optional<DisplayVideoCommand> ParseSetDisplayVideoPayload(
    const std::string& payload_json) {
  if (payload_json.size() > kMaxPayloadBytes) return std::nullopt;
  DisplayVideoCommand cmd;
  try {
    const nlohmann::json payload = nlohmann::json::parse(payload_json);
    if (!payload.is_object()) return std::nullopt;
    if (!payload.contains("device") || !payload.at("device").is_string()) {
      return std::nullopt;
    }
    cmd.device = payload.at("device").get<std::string>();
    if (!IsGdiDeviceName(cmd.device)) return std::nullopt;

    bool clear = false;
    if (payload.contains("clear")) {
      if (!payload.at("clear").is_boolean()) return std::nullopt;
      clear = payload.at("clear").get<bool>();
    }

    const bool has_path = payload.contains("path");
    if (clear) {
      if (has_path) return std::nullopt;
      cmd.clear = true;
      return cmd;
    }
    if (!has_path || !payload.at("path").is_string()) return std::nullopt;
    cmd.path = payload.at("path").get<std::string>();
    if (cmd.path.empty()) return std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
  return cmd;
}

}  // namespace k6wp
