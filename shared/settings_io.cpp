#include "settings_io.hpp"

#include "config_schema.hpp"

#include <cstdint>
#include <fstream>
#include <system_error>

namespace k6wp::detail {

std::string ReadFile(const std::filesystem::path& path, const char* what) {
  std::error_code ec;
  const std::uintmax_t size = std::filesystem::file_size(path, ec);
  if (!ec && size > kMaxConfigBytes) {
    throw ConfigError(std::string(what) + " exceeds maximum size (" +
                      std::to_string(kMaxConfigBytes) + " bytes): " +
                      path.string());
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw ConfigError(std::string("cannot open ") + what + ": " +
                      path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

void BackupFile(const std::filesystem::path& path) noexcept {
  std::error_code ec;
  std::filesystem::path bak = path;
  bak += L".bak";
  std::filesystem::copy_file(path, bak,
                             std::filesystem::copy_options::overwrite_existing,
                             ec);
}

}  // namespace k6wp::detail
