#pragma once

// Engine:FirstFrame benchmark marker (Todo 12).
//
// MarkFirstFrame() emits a single "Engine:FirstFrame qpc:<ticks> qpf:<freq>"
// line to stdout when the first frame renders. Thread-safe; first caller wins.
// The bench scripts (tools/bench_startup.ps1) parse this line to measure
// process-start-to-first-frame latency.

namespace k6wp {

// Emit the Engine:FirstFrame marker to stdout exactly once.
// Uses QueryPerformanceCounter for high-resolution timestamp.
void MarkFirstFrame();

}  // namespace k6wp
