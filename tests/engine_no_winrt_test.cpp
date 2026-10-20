// engine_no_winrt_test.cpp - proves the resident engine stays free of WinRT.
//
// The per-user still feature (launcher/still_apply.cpp) is the ONLY place that
// includes C++/WinRT (LockScreen::SetImageFileAsync) and it lives in the
// one-shot K6WP.exe helper. The engine must never link it: no combase.dll
// dependency, no api-ms-win-core-winrt-* dependency and no Ro*/Windows*String
// activation imports.
//
// This test parses the PE import table directly (no dumpbin dependency, so it
// runs from any ctest invocation) and scans BOTH binaries:
//   argv[1] = engine.exe  -> must be clean
//   sibling K6WP.exe      -> must carry the WinRT imports (positive control:
//                            proves the detector can actually see them).
//
// No external test framework: plain Check() counter; exit 0 = pass.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <filesystem>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("[PASS] %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("[FAIL] %s\n", name.c_str());
  }
}

std::uint16_t U16(const std::vector<std::uint8_t>& b, std::size_t off) {
  if (off + 2 > b.size()) return 0;
  return static_cast<std::uint16_t>(b[off] | (b[off + 1] << 8));
}

std::uint32_t U32(const std::vector<std::uint8_t>& b, std::size_t off) {
  if (off + 4 > b.size()) return 0;
  return static_cast<std::uint32_t>(b[off]) |
         (static_cast<std::uint32_t>(b[off + 1]) << 8) |
         (static_cast<std::uint32_t>(b[off + 2]) << 16) |
         (static_cast<std::uint32_t>(b[off + 3]) << 24);
}

std::uint64_t U64(const std::vector<std::uint8_t>& b, std::size_t off) {
  return static_cast<std::uint64_t>(U32(b, off)) |
         (static_cast<std::uint64_t>(U32(b, off + 4)) << 32);
}

std::string Lower(std::string text) {
  for (char& c : text) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return text;
}

std::string ReadCString(const std::vector<std::uint8_t>& b, std::size_t off) {
  std::string text;
  while (off < b.size() && b[off] != 0 && text.size() < 512) {
    text.push_back(static_cast<char>(b[off++]));
  }
  return text;
}

struct Section {
  std::uint32_t va = 0;
  std::uint32_t vsize = 0;
  std::uint32_t raw_size = 0;
  std::uint32_t raw_ptr = 0;
};

struct Imports {
  bool valid = false;
  bool pe32plus = false;
  std::vector<Section> sections;
  std::uint32_t dir_rva = 0;
  std::uint32_t dir_size = 0;
  std::vector<std::string> dlls;
  std::vector<std::string> functions;
};

bool RvaToOffset(const Imports& pe, std::uint32_t rva, std::size_t& out) {
  for (const Section& s : pe.sections) {
    const std::uint32_t span = s.vsize > s.raw_size ? s.vsize : s.raw_size;
    if (rva >= s.va && rva < s.va + span) {
      const std::uint32_t delta = rva - s.va;
      if (delta >= s.raw_size) return false;  // inside the zero-filled tail
      out = static_cast<std::size_t>(s.raw_ptr) + delta;
      return true;
    }
  }
  return false;
}

Imports ParseImports(const std::vector<std::uint8_t>& b) {
  Imports pe;
  if (b.size() < 0x40 || b[0] != 'M' || b[1] != 'Z') return pe;
  const std::size_t pe_off = U32(b, 0x3C);
  if (pe_off + 24 > b.size()) return pe;
  if (b[pe_off] != 'P' || b[pe_off + 1] != 'E' || b[pe_off + 2] != 0 ||
      b[pe_off + 3] != 0) {
    return pe;
  }
  const std::uint16_t num_sections = U16(b, pe_off + 6);
  const std::uint16_t opt_size = U16(b, pe_off + 20);
  const std::size_t opt = pe_off + 24;
  if (opt + 2 > b.size()) return pe;
  const std::uint16_t magic = U16(b, opt);
  if (magic != 0x10B && magic != 0x20B) return pe;
  pe.pe32plus = magic == 0x20B;
  const std::size_t dd = opt + (pe.pe32plus ? 112 : 96);
  if (dd + 16 > b.size()) return pe;
  pe.dir_rva = U32(b, dd + 8);
  pe.dir_size = U32(b, dd + 12);

  const std::size_t sec = opt + opt_size;
  for (std::uint16_t i = 0; i < num_sections; ++i) {
    const std::size_t s = sec + static_cast<std::size_t>(i) * 40;
    if (s + 40 > b.size()) return pe;
    Section section;
    section.vsize = U32(b, s + 8);
    section.va = U32(b, s + 12);
    section.raw_size = U32(b, s + 16);
    section.raw_ptr = U32(b, s + 20);
    pe.sections.push_back(section);
  }
  if (pe.dir_rva == 0 || pe.dir_size == 0) {
    pe.valid = true;  // valid image, simply no imports
    return pe;
  }
  pe.valid = true;

  // Import descriptors are 20 bytes: OFT, timestamp, forwarder, Name, FT.
  constexpr std::size_t kDescriptorBytes = 20;
  constexpr std::size_t kMaxDescriptors = 4096;
  constexpr std::size_t kMaxThunks = 200000;
  std::size_t desc = 0;
  if (!RvaToOffset(pe, pe.dir_rva, desc)) return pe;
  for (std::size_t i = 0; i < kMaxDescriptors; ++i, desc += kDescriptorBytes) {
    if (desc + kDescriptorBytes > b.size()) break;
    const std::uint32_t oft = U32(b, desc);
    const std::uint32_t name_rva = U32(b, desc + 12);
    const std::uint32_t ft = U32(b, desc + 16);
    if (name_rva == 0 && ft == 0) break;  // terminator
    std::size_t name_off = 0;
    if (name_rva != 0 && RvaToOffset(pe, name_rva, name_off)) {
      const std::string dll = ReadCString(b, name_off);
      if (!dll.empty()) pe.dlls.push_back(dll);
    }
    const std::uint32_t thunk_rva = oft != 0 ? oft : ft;
    std::size_t thunk = 0;
    if (thunk_rva == 0 || !RvaToOffset(pe, thunk_rva, thunk)) continue;
    const std::size_t width = pe.pe32plus ? 8 : 4;
    for (std::size_t t = 0; t < kMaxThunks; ++t, thunk += width) {
      if (thunk + width > b.size()) break;
      const std::uint64_t value =
          pe.pe32plus ? U64(b, thunk) : U32(b, thunk);
      if (value == 0) break;
      const bool by_ordinal =
          pe.pe32plus ? ((value >> 63) != 0) : ((value >> 31) != 0);
      if (by_ordinal) continue;
      const std::uint32_t fn_rva = static_cast<std::uint32_t>(value);
      std::size_t fn_off = 0;
      if (!RvaToOffset(pe, fn_rva, fn_off)) continue;
      const std::string fn = ReadCString(b, fn_off + 2);  // skip the hint
      if (!fn.empty()) pe.functions.push_back(fn);
    }
  }
  return pe;
}

bool HasDllContaining(const Imports& pe, const std::string& needle) {
  for (const std::string& dll : pe.dlls) {
    if (Lower(dll).find(needle) != std::string::npos) return true;
  }
  return false;
}

bool HasFunction(const Imports& pe, const char* name) {
  for (const std::string& fn : pe.functions) {
    if (fn == name) return true;
  }
  return false;
}

bool HasAnyWinrtFunction(const Imports& pe) {
  static const char* const kFns[] = {
      "RoGetActivationFactory", "RoActivateInstance", "RoInitialize",
      "RoUninitialize",         "WindowsCreateString", "WindowsDeleteString",
      "WindowsGetStringRawBuffer",
  };
  for (const char* fn : kFns) {
    if (HasFunction(pe, fn)) return true;
  }
  return false;
}

bool HasWinrtDependency(const Imports& pe) {
  return HasDllContaining(pe, "combase.dll") ||
         HasDllContaining(pe, "api-ms-win-core-winrt") ||
         HasAnyWinrtFunction(pe);
}

std::vector<std::uint8_t> ReadFileBytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

void PrintImports(const char* label, const Imports& pe) {
  std::printf("%s imports (%zu DLLs):\n", label, pe.dlls.size());
  for (const std::string& dll : pe.dlls) std::printf("  %s\n", dll.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("usage: engine_no_winrt_test <engine.exe>\n");
    return 2;
  }
  const std::filesystem::path engine_path = argv[1];

  const std::vector<std::uint8_t> bytes = ReadFileBytes(engine_path);
  Check(!bytes.empty(), "engine.exe is readable");
  const Imports engine = ParseImports(bytes);
  Check(engine.valid, "engine.exe is a valid PE image");
  Check(!engine.dlls.empty(), "engine.exe imports at least one DLL");
  Check(HasDllContaining(engine, "kernel32.dll"),
        "engine.exe imports kernel32.dll (parser sanity)");
  PrintImports("engine.exe", engine);

  Check(!HasDllContaining(engine, "combase.dll"),
        "engine.exe does not import combase.dll (WinRT activation)");
  Check(!HasDllContaining(engine, "api-ms-win-core-winrt"),
        "engine.exe does not import api-ms-win-core-winrt-*");
  Check(!HasAnyWinrtFunction(engine),
        "engine.exe does not import Ro*/Windows*String activation functions");
  Check(!HasWinrtDependency(engine),
        "engine.exe has no WinRT dependency at all");

  // Positive control: the helper K6WP.exe must carry what the engine must not.
  const std::filesystem::path helper_path =
      engine_path.parent_path() / "K6WP.exe";
  std::error_code ec;
  if (std::filesystem::exists(helper_path, ec)) {
    const std::vector<std::uint8_t> helper_bytes = ReadFileBytes(helper_path);
    const Imports helper = ParseImports(helper_bytes);
    Check(helper.valid, "K6WP.exe is a valid PE image");
    PrintImports("K6WP.exe", helper);
    Check(HasWinrtDependency(helper),
          "positive control: K6WP.exe carries the WinRT dependency");
  } else {
    std::printf(
        "[SKIP] K6WP.exe not next to engine.exe; positive control skipped\n");
  }

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
