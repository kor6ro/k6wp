#pragma once

// Friendly compression UX (Step 7.3): plain-language mapping for compressor
// technical errors plus the long-video consent gate. Full technical strings
// go to the log view only — never into message boxes or status labels.

#include <QString>

namespace k6wp {

// Maps a compressor technical error to an Indonesian plain-language message.
// Unknown input falls back to a generic message (never empty, never JSON).
QString FriendlyCompressError(const QString& technical);

}  // namespace k6wp
