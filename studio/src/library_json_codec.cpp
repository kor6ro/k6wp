#include "library_json_codec.hpp"

namespace k6wp {

nlohmann::json EntryToJson(const LibraryEntry& e) {
  nlohmann::json j;
  j["src"] = std::filesystem::path(e.src).u8string();
  j["dst"] = std::filesystem::path(e.dst).u8string();
  j["res"] = e.res;
  j["fps"] = e.fps;
  j["crf"] = e.crf;
  j["encoder"] = e.encoder;
  j["mtime"] = e.mtime;
  j["size"] = e.size;
  j["duration"] = e.duration;
  j["codec"] = e.codec;
  j["width"] = e.width;
  j["height"] = e.height;
  j["thumb"] = std::filesystem::path(e.thumb).u8string();
  return j;
}

LibraryEntry EntryFromJson(const nlohmann::json& j) {
  LibraryEntry e;
  // value() on a wrong-typed field throws type_error (a json::exception);
  // the caller converts it to LibraryError.
  e.src = std::filesystem::u8path(j.value("src", ""));
  e.dst = std::filesystem::u8path(j.value("dst", ""));
  e.res = j.value("res", "0x0");
  e.fps = j.value("fps", 30);
  e.crf = j.value("crf", 23);
  e.encoder = j.value("encoder", "libx264");
  e.mtime = j.value("mtime", std::int64_t{0});
  e.size = j.value("size", std::uint64_t{0});
  e.duration = j.value("duration", 0.0);
  e.codec = j.value("codec", "");
  e.width = j.value("width", 0);
  e.height = j.value("height", 0);
  e.thumb = std::filesystem::u8path(j.value("thumb", ""));
  e.broken = false;  // computed at ListItems() time, never persisted
  return e;
}

}  // namespace k6wp
