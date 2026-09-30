#pragma once

// User-facing wording for the engine / IPC / preview error paths (1.2.1).
//
// studio/src/compress_errors.cpp already owns the compressor's mapping
// (FriendlyCompressError); the engine, the IPC transport and the mpv preview
// had no equivalent, so StudioBridge showed their raw technical strings
// verbatim. Those strings carry absolute paths, .exe filenames, log prefixes
// ("Apply:") and libmpv's English error text — none of which a user can act
// on, and several of which leak the install layout.
//
// Contract, identical for every function here:
//   * NEVER empty;
//   * NEVER echoes the technical string it was given;
//   * NEVER contains a path, a .exe name, or a log prefix;
//   * the unmatched fallback is a fixed, actionable sentence — so a new
//     technical string can degrade to "generic advice", never to a raw dump.
// The technical string always goes to the log pane instead (AppendLog /
// stderr); these functions are only for what the user reads.
//
// Not a second compress table: FriendlyCompressError stays the compressor's
// mapping and is extended in place (compress_errors.cpp). This module covers
// the two domains that have no mapping at all.

#include <QString>

namespace k6wp {

// ApplyManager / engine-start failures. `technical` is one of the ApplyManager
// error_out strings, or an empty string when the caller had nothing to report.
QString FriendlyApplyError(const QString& technical);

// IpcResult::error — the pipe transport / engine rejection text.
QString FriendlyIpcError(const QString& technical);

// libmpv mpv_error_string() text (or empty when mpv gave none).
QString FriendlyPreviewError(const QString& technical);

}  // namespace k6wp
