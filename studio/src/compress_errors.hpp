#pragma once

// Friendly compression UX (Step 7.3): plain-language mapping for compressor
// technical errors plus the long-video consent gate. Full technical strings
// go to the log view only — never into message boxes or status labels.

#include <QString>

class QWidget;

namespace k6wp {

// Maps a compressor technical error to an Indonesian plain-language message.
// Unknown input falls back to a generic message (never empty, never JSON).
QString FriendlyCompressError(const QString& technical);

// Long-video (>600 s) consent gate, used by every enqueue origin. Returns
// true when the job may enter the queue (sets req.force upstream on true).
// A "don't ask again this session" checkbox suppresses repeat prompts.
bool AskLongVideoConsent(QWidget* parent, const QString& src,
                         double duration_sec);

}  // namespace k6wp
