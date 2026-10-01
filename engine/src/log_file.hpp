#pragma once

#include <cstdarg>

// Engine file-log sink (Todo 11, GUI-subsystem follow-up).
// GUI-subsystem exes have no console, so EngineApp::Log / tray Log route
// every line here. The stdout mirror in each Log() is compiled in only with
// K6WP_VERBOSE (root CMake option, default OFF); production builds are
// console-clean. The file is
// %LOCALAPPDATA%/K6WP/engine.log, opened once (lazily, on the first line)
// in append mode.
//
// Batching: lines accumulate in an 8 KiB buffer and hit disk on flush —
// either when the buffer fills or when an important line arrives (warn /
// error levels and state transitions flush immediately, so a `taskkill /F`
// from RestartEngine never loses the diagnostic that explains it).
// FlushEngineLog() drains the buffer; call it on normal shutdown and the
// IPC quit path. The crash filter uses FlushEngineLogFromCrash() (try-lock,
// never blocks).
//
// Never throws, never fatal: when the log dir/file is unwritable the line
// goes to OutputDebugStringW instead and execution continues (tray stays).
namespace k6wp {

void AppendEngineLogLine(const char* line, bool important = false);
// One timestamped log line ("[HH:MM:SS.mmm] msg"), the shared sink for engine
// TUs: the file mutex and the K6WP_VERBOSE stderr mirror live here instead of
// in four private copies that had already drifted. `important` flushes at once.
void EngineLogfV(const char* fmt, va_list args, bool important = false);
void FlushEngineLog();
// Crash-filter variant: best-effort flush that never blocks. Safe to call
// from SetUnhandledExceptionFilter (returns at once when the log mutex is
// held by the crashed thread).
void FlushEngineLogFromCrash();

}  // namespace k6wp
