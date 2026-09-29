// engine/src/ipc_marshal.cpp
// Worker-side validators for the PostMessage-marshaled IPC paths (see the
// header for the threading contract).
#include "ipc_marshal.hpp"

#include <filesystem>

#include "thirdparty/json.hpp"

namespace k6wp {

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
    id = value->get<int>();
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

}  // namespace k6wp
