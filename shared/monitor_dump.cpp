// monitor_dump - prints the monitor list as JSON to stdout and exits 0.
// Output: {"monitors":[{"id":0,"x":0,"y":0,"width":1920,"height":1080,
// "is_primary":true,"device_name":"\\\\.\\DISPLAY1","orientation":0,
// "refresh_hz":60,"scale_pct":100},...],"virtual_screen":{"x":0,"y":0,
// "width":3840,"height":1080}}
// Prints {"monitors":[],...} (and exits 0) when no monitors are present.
#include "monitor_util.hpp"

#include <cstdio>
#include <string>

int main() {
  k6wp::SetProcessDpiAwarenessContextPMDA();

  const auto monitors = k6wp::ListMonitors();
  const std::string out = k6wp::FormatMonitorsJson(monitors) + "\n";

  std::fputs(out.c_str(), stdout);
  return 0;
}