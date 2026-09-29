#pragma once

#include <string>

namespace k6wp {

// Autostart via HKCU\Software\Microsoft\Windows\CurrentVersion\Run (Todo 36).
//
// The Run value name is "K6WP"; the value is resolved by AutostartCommand():
//   - If the caller is K6WP.exe: use K6WP.exe sibling with "--engine --silent"
//   - Otherwise (e.g. studio.exe): prefer <exe_dir>/K6WP.exe when it exists with
//     "--engine --silent"; legacy fall-back to sibling engine.exe + --minimized
//     only when K6WP.exe is absent. HKCU needs no admin rights. Header stays
//   windows.h-free (same pattern as monitor_util / config_schema /
//   cache_manager): Win32 lives only in autostart.cpp, so Qt Studio can include
//   this header with no windows.h pollution.

// Registry value name under the HKCU...\Run key.
inline constexpr wchar_t kAutostartValueName[] = L"K6WP";

// True when the K6WP Run value exists. Never throws.
bool IsAutostart() noexcept;

// Resolves the command line stored in the Run value:
// `"<K6WP.exe path>" --engine --silent`, with legacy fall-back to
// `"<engine.exe path>" --minimized` when no K6WP.exe sits next to the
// current process exe. Never throws.
std::wstring AutostartCommand();

// Creates (enable=true) or deletes (enable=false) the Run value.
// Deleting a missing value (already OFF) succeeds. Returns false with a
// human-readable UTF-8 message in *error_out (may be nullptr) when the
// registry call fails — e.g. access denied — so the UI can show it directly.
bool SetAutostart(bool enable, std::string* error_out);

}  // namespace k6wp
