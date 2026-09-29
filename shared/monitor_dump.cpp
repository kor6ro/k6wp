// monitor_dump - prints the monitor list as JSON to stdout and exits 0.
// Output: [{"id":0,"width":1920,"height":1080,"is_primary":true},...]
// Prints [] (and exits 0) when no monitors are present.
#include "monitor_util.hpp"

#include <cstdio>
#include <string>

int main() {
  k6wp::SetProcessDpiAwarenessContextPMDA();

  const auto monitors = k6wp::ListMonitors();

  std::string out = "[";
  for (std::size_t i = 0; i < monitors.size(); ++i) {
    if (i > 0) out += ",";
    const auto& m = monitors[i];
    out += "{\"id\":" + std::to_string(m.id) +
           ",\"width\":" + std::to_string(m.width) +
           ",\"height\":" + std::to_string(m.height) +
           ",\"is_primary\":" + (m.is_primary ? "true" : "false") + "}";
  }
  out += "]\n";

  std::fputs(out.c_str(), stdout);
  return 0;
}